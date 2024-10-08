/*

MIT License

Copyright (c) 2024 Dementive

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

------------------------------------------------------------------------------

GDExpr is a scripting language for Godot. This header file contains the compiler, interpreter, and runtime.
The interface and most of the implementation of the language is in the GDExpr singleton class, which can be accessed from any GDExtension or gdscript.

Unlike most sane languages that compile to bytecode or machine code GDExpr instead compiles to Godot expressions.
Godot expressions are extremely simple, they only has 5 operators (+,-,/,*,%), none of which have any special behavior the GDExpr compiler has to care about.

The killer feature of Godot expressions that made me want to compile to them is their ability to call any @GlobalScope functions or any function passed in on a object pointer.
All you have to do is write 1 line of code to bind the function in C++. In gdscript you don't have to write any bindings at all, the functions bind automatically!
This makes your C++ code considerably simpler than most other scripting solutions would...
Compare this to creating function bindings from C->Lua: https://chsasank.com/lua-c-wrapping.html
Additionally godot expressions natively support every Variant type and all operations on them out of the box...which means you can do pretty much anything with any type.
They also fully implement conditional logic and pretty much all the ways you can use it.

All of the features godot expressions have don't have to be implemented in GDExpr at all since godot expressions are able to handle this for us.
This makes the languague implementation super simple in addition to having a simple API and gdexpr syntax also being...simple.

Another great feature of Godot expressions have is they will pretty much never cause crashes no matter how stupid the input you throw at it is, and sometimes it even
gives sensible error messages! Because of this the GDExpr compiler generally just does not care at all if the text it is compiling is actually valid, since the Godot
expressions compiler *should* handle all of this for us. In my entire time writing this there hasn't been any input I could throw at it that caused a crash.

Basic checks to ensure the compiler itself will never crash are still done...of course but ensuring the syntax we are compiling is actually valid isn't something this
compiler really worries about.

Since the compiler also manages the runtime if the user makes syntax errors the compiler will notify what expressions failed, the file it
failed in and the error message the Godot expression compiler emitted.
The big thing the compiler must ensure is that all gdexpr compiles to the exact equivalent sequence of godot expressions, otherwise this would be a pain to debug for the user

Compiling to Godot expressions has a lot of...fun things to work around that a normal compiler would never have to worry about.

For example, the only possible way to implement if statements and loops was by doing it at compile time and adding a "comptime" keyword that works kind of like Zig comptime.
I would really prefer to not have to implement comptime because it is a complex concept for scripting language syntax
but there is actually no other possible way to do ifs or loops that I could thin of.

Also any function that returns void in the runtime will kill the execution of the current expression, preventing other subsequent statements from being run, which sucks.
This isn't something the compiler can fix either, it can help mitigate some of the side effects of it, but the user still has to make their functions return valid Variants

Because a lot of the normal features of a programming language are not actually possible to achieve in the compiled code a lot of these features have to somehow be
implemented by the compiler, at compile time. Then the compiled code has to somehow be assembled into a sequence of Godot expressions that are executed in the same way the
GDExpr syntax allows. So the compiler has to also do a lot of really weird stuff to work around these limitations.
*/

#ifndef GDExpr_H
#define GDExpr_H

#include "godot_cpp/classes/dir_access.hpp"
#include "godot_cpp/classes/expression.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/time.hpp"
#include "godot_cpp/templates/hash_set.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

using namespace godot;

namespace gdexpr {

// NOTE: Uncomment this if debugging the compiler, it will compile with debugging and timing logging.
//#define GDEXPR_COMPILER_DEBUG
//#define GDEXPR_COMPILER_TIMING_DEBUG

// NOTE: Uncomment this to enable logging when debugging the comptime parts of the compiler.
//#define GDEXPR_COMPTIME_DEBUG

// Measure execution time of m_code with a label m_thing_to_time in microseconds and print the result.
#define TIME_MICRO(m_thing_to_time, m_code)                                                                                                                                   \
	uint64_t X_##m_thing_to_time##_start = Time::get_singleton()->get_ticks_usec();                                                                                           \
	m_code;                                                                                                                                                                   \
	uint64_t X_##m_thing_to_time##_end = Time::get_singleton()->get_ticks_usec();                                                                                             \
	UtilityFunctions::print("Total time taken to run ", #m_thing_to_time, ": ", (X_##m_thing_to_time##_end - X_##m_thing_to_time##_start), " microseconds");

// Measure execution time of m_code with a label m_thing_to_time in seconds and print the result.
#define TIME_SECONDS(m_thing_to_time, m_code)                                                                                                                                 \
	uint64_t X_##m_thing_to_time##_start = Time::get_singleton()->get_ticks_usec();                                                                                           \
	m_code;                                                                                                                                                                   \
	uint64_t X_##m_thing_to_time##_end = Time::get_singleton()->get_ticks_usec();                                                                                             \
	UtilityFunctions::print("Total time taken to run ", #m_thing_to_time, ": ", ((X_##m_thing_to_time##_end - X_##m_thing_to_time##_start) / 1000000.0), " seconds");

// If m_code can't be passed into TIME_MICRO or TIME_SECONDS because it changes the control flow use this instead...
#define TIME_START(m_thing_to_time) uint64_t X_##m_thing_to_time##_start = Time::get_singleton()->get_ticks_usec();

// Call at the end of the code TIME_START is testing to measure time in microseconds.
#define TIME_MICRO_END(m_thing_to_time)                                                                                                                                       \
	uint64_t X_##m_thing_to_time##_end = Time::get_singleton()->get_ticks_usec();                                                                                             \
	UtilityFunctions::print("Total time taken to run ", #m_thing_to_time, ": ", (X_##m_thing_to_time##_end - X_##m_thing_to_time##_start), " microseconds");

// Call at the end of the code TIME_START is testing to measure time in seconds.
#define TIME_SECONDS_END(m_thing_to_time)                                                                                                                                     \
	uint64_t X_##m_thing_to_time##_end = Time::get_singleton()->get_ticks_usec();                                                                                             \
	UtilityFunctions::print("Total time taken to run ", #m_thing_to_time, ": ", ((X_##m_thing_to_time##_end - X_##m_thing_to_time##_start) / 1000000.0), " seconds");

// This class is essentially the GDExpr runtime, all scripts that call GDExpr must inhereit from GDExprScript.
class GDExprScript : public RefCounted {
	GDCLASS(GDExprScript, RefCounted)

private:
	Dictionary runtime_variables;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("get_var"), &GDExprScript::get_var);
		ClassDB::bind_method(D_METHOD("set_var", "var_name", "var_value"), &GDExprScript::set_var);
	}

public:
	GDExprScript() {}

	// IMPORTANT NOTE: If the base_instance passed into a GDExpr expression does not have get_var, set_var and a Dictionary class variables in GDExpr will not work and will
	// cause the expression to fail! So be sure to inherit from GDExprScript.
	Variant get_var(String var_name) { return runtime_variables[var_name]; }
	int set_var(String var_name, Variant var_value) {
		runtime_variables[var_name] = var_value;
		return 0;
	}
};

struct SortByLongest {
	bool operator()(const String &a, const String &b) const { return a.length() > b.length(); }
};

_ALWAYS_INLINE_ PackedStringArray whitespace_split(const String &p_string, const char *p_splitter) {
	PackedStringArray ret;
	bool inside_quote = false;
	bool quote_type = false; // false for single quote, true for double quote
	String current_word;
	bool comment_started = false;

	for (int i = 0; i < p_string.length(); i++) {
		if (p_string[i] == '#' && !inside_quote && !comment_started) {
			comment_started = true;
		} else if (p_string[i] == '\n' && comment_started) {
			comment_started = false;
		} else if (!comment_started) {
			if (p_string[i] == '"' && !inside_quote) {
				inside_quote = true;
				quote_type = true;
				current_word += p_string[i];
			} else if (p_string[i] == '"' && inside_quote && quote_type) {
				inside_quote = false;
				current_word += p_string[i];
			} else if (p_string[i] == '\'' && !inside_quote) {
				inside_quote = true;
				quote_type = false;
				current_word += p_string[i];
			} else if (p_string[i] == '\'' && inside_quote && !quote_type) {
				inside_quote = false;
				current_word += p_string[i];
			} else if (p_string[i] == *p_splitter && !inside_quote) {
				if (!current_word.is_empty()) {
					ret.push_back(current_word);
					current_word = "";
				}
			} else {
				current_word += p_string[i];
			}
		}
	}

	if (!current_word.is_empty()) {
		ret.push_back(current_word);
	}

	return ret;
}

// Same as the above function but returns a String. Used in the config script compiler because it is considerably faster than joining PackedStringArrays
// TODO - merge these 2 functions using a template...they do the same thing.
_ALWAYS_INLINE_ String whitespace_split_string(const String &p_string, const char *p_splitter) {
	String result;
	bool inside_quote = false;
	bool quote_type = false; // false for single quote, true for double quote
	String current_word;
	bool comment_started = false;

	for (int i = 0; i < p_string.length(); i++) {
		if (p_string[i] == '#' && !inside_quote && !comment_started) {
			comment_started = true;
		} else if (p_string[i] == '\n' && comment_started) {
			comment_started = false;
		} else if (!comment_started) {
			if (p_string[i] == '"' && !inside_quote) {
				inside_quote = true;
				quote_type = true;
				current_word += p_string[i];
			} else if (p_string[i] == '"' && inside_quote && quote_type) {
				inside_quote = false;
				current_word += p_string[i];
			} else if (p_string[i] == '\'' && !inside_quote) {
				inside_quote = true;
				quote_type = false;
				current_word += p_string[i];
			} else if (p_string[i] == '\'' && inside_quote && !quote_type) {
				inside_quote = false;
				current_word += p_string[i];
			} else if (p_string[i] == *p_splitter && !inside_quote) {
				if (!current_word.is_empty()) {
					result += current_word + " ";
					current_word = "";
				}
			} else {
				current_word += p_string[i];
			}
		}
	}

	if (!current_word.is_empty()) {
		result += current_word;
	}

	return result.strip_edges();
}

class GDExpr : public RefCounted {
	GDCLASS(GDExpr, RefCounted)

public:
	GDExpr() {
		expression = memnew(Expression);
		ERR_FAIL_COND(singleton != nullptr);
		singleton = this;
	}

	~GDExpr() {
		memdelete(expression);
		expression = nullptr;
		ERR_FAIL_COND(singleton != this);
		singleton = nullptr;
	}

private:
	inline static GDExpr *singleton = nullptr;
	static GDExpr *get_singleton();

	Expression *expression = nullptr;
	Ref<GDExprScript> base_instance = nullptr;
	Vector<String> variables;
	Dictionary comptime_variables;
	HashSet<String> current_includes;
	String file_to_compile;
	String current_variable_name;
	String current_variable_value;
	Array conditional_stack;
	bool is_inside_multiline_declaration = false;
	bool is_inside_condition = false;
	Array expression_inputs;

	String parse_directory(String dir_path) {
		Ref<DirAccess> dir = DirAccess::open(dir_path);
		PackedStringArray files = dir->get_files();
		String expressions;
		for (int i = 0; i < files.size(); ++i) {
			String file = files[i];
			if (file.get_extension() != "gdexpr")
				continue;

			String expr = parse_file(file);
			if (expr.is_empty())
				continue;

			// Add expression and seperator to all tokens except the last one.
			if (i != files.size() - 1) {
				expressions += expr;
			} else {
				expressions += expr + "\n---";
			}
		}

		return expressions;
	}

	String parse_file(String file_path) {
		Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::READ);
		ERR_FAIL_NULL_V(file, String());

		return file->get_as_text(true);
	}

	String parse_include_file(String file_path) {
		Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::READ);
		ERR_FAIL_NULL_V(file, String());

		return file->get_as_text(true);
	}

	String check_for_comptime_vars(String var_token) {
		Array comptime_vars_keys = comptime_variables.keys();
		comptime_vars_keys.sort_custom(Callable(this, "sort_by_longest")); // TODO OPTIMIZE - is it possible/faster to sort at insertion time???

		for (int i = 0; i < comptime_vars_keys.size(); ++i) {
			String variable = comptime_vars_keys[i];
			if (var_token.contains(variable)) {
				var_token = var_token.replace(variable, comptime_variables[variable]);
			}
		}

		return var_token;
	}

	String get_comptime_var_value(PackedStringArray &line_tokens) {
		String variable_name = line_tokens[2];
		String variable_value;
		for (int j = 4; j < line_tokens.size(); ++j) {
			variable_value += line_tokens[j] + " ";
		}

		variable_value = check_for_comptime_vars(variable_value);
		return variable_value;
	}

	void preprocess_variables(PackedStringArray &line_tokens, PackedStringArray &expression_tokens) {
		String variable_name = line_tokens[1];
		String variable_value;

		if (line_tokens[line_tokens.size() - 1].ends_with("(")) {
			is_inside_multiline_declaration = true;
		}

		for (int j = 3; j < line_tokens.size(); ++j) {
			variable_value += line_tokens[j] + " ";
		}

		for (String variable : variables) {
			if (variable_value.contains(variable)) {
				variable_value = variable_value.replace(variable, vformat("get_var(\"%s\")", variable));
			}
		}

		variables.append(variable_name);
		variables.sort_custom<SortByLongest>(); // TODO OPTIMIZE - is it possible/faster to sort at insertion time???

		if (!is_inside_multiline_declaration) {
			String var_token = vformat("set_var(\"%s\", %s)+", variable_name, variable_value);
			var_token = check_for_comptime_vars(var_token);

			expression_tokens.append(var_token);
			line_tokens.append("---");
		} else {
			current_variable_name = variable_name;
			current_variable_value = variable_value;
		}
	}

	void reset_to_default_state() {
		file_to_compile = "";
		current_variable_name = "";
		current_variable_value = "";
		is_inside_multiline_declaration = false;
		is_inside_condition = false;

		conditional_stack.clear();
		comptime_variables.clear();
		variables.clear();
		current_includes.clear();
		expression_inputs.clear();
	}

	Variant comptime_execute(String expression_to_parse) {
		// Execute an expression at comptime

		expression->parse(expression_to_parse);
		Variant result = expression->execute(expression_inputs, *base_instance);

		if (expression->has_execute_failed()) {
			UtilityFunctions::printerr(vformat("[%s] - ", file_to_compile), "GDExpr comptime expression: \"", expression_to_parse,
					"\" failed to execute with error: ", expression->get_error_text());
		}

#ifdef GDEXPR_COMPTIME_DEBUG
		UtilityFunctions::print("COMPTIME EXPR TO PARSE: ", expression_to_parse);
		UtilityFunctions::print("COMPTIME EXPR RESULT: ", result);
#endif
		return result;
	}

	Array _execute_expressions(PackedStringArray compiled_expression, String file_to_compile, bool is_running_as_interpreter) {
#ifdef GDEXPR_COMPILER_TIMING_DEBUG
		TIME_START(gdexpr_execution)
#endif

		Array results;
		for (int i = 0; i < compiled_expression.size(); ++i) {
			String expression_to_parse = compiled_expression[i];

			expression->parse(expression_to_parse);
			Variant result = expression->execute(expression_inputs, *base_instance);

			if (expression->has_execute_failed()) {
				if (is_running_as_interpreter) {
					UtilityFunctions::printerr(
							vformat("[%d] - ", i + 1), "GDExpr expression: \"", expression_to_parse, "\" failed to execute with error: ", expression->get_error_text());
				} else {
					UtilityFunctions::printerr(vformat("[%s:%d] - ", file_to_compile, i + 1), "GDExpr expression: \"", expression_to_parse,
							"\" failed to execute with error: ", expression->get_error_text());
				}
				continue;
			}

#ifdef GDEXPR_COMPILER_DEBUG
			UtilityFunctions::print("EXPR TO PARSE: ", expression_to_parse);
			UtilityFunctions::print("EXPR RESULT: ", result);
#endif

			results.push_back(result);
		}

		reset_to_default_state();

#ifdef GDEXPR_COMPILER_TIMING_DEBUG
		TIME_MICRO_END(gdexpr_execution)
#endif
		return results;
	}

	PackedStringArray compile(String input_string) {
		PackedStringArray compiled_expressions;
		PackedStringArray expression_tokens;
		Dictionary macro_defines;
		Array macro_define_keys;
		PackedStringArray lines = input_string.split("\n");
		bool is_config_script = lines.size() > 0 and lines[0] == "@config" ? true : false;

		// Stripped down version of the compiler that only has a break statement.
		// This is ideal for use with scripts that don't use any variables or conditional logic and will be significantly faster to compile than the full language.
		if (is_config_script) {
			String expr_string = "";
			for (int i = 1; i < lines.size(); ++i) {
				String line = lines[i];

				if (line.is_empty())
					continue;

				// Split by white space
				String line_tokens = whitespace_split_string(line, " ");
				if (line_tokens.is_empty())
					continue;

				// Break expression
				if (line_tokens.begins_with("break") or line_tokens.begins_with("---")) {
					if (!expr_string.is_empty()) {
						compiled_expressions.append(expr_string);
						expr_string = "";
						continue;
					}
				}

				// expression_tokens.append(String().join(line_tokens));
				expr_string += line_tokens;
			}

			if (!expr_string.is_empty())
				compiled_expressions.append(expr_string);

			return compiled_expressions;
		}

		// Full compiler with all gdexpr features
		for (int i = 0; i < lines.size(); ++i) {
			String line = lines[i];

			if (line.is_empty())
				continue;

			// Split by white space
			line = line.strip_edges();
			PackedStringArray line_tokens = whitespace_split(line, " ");
			if (line_tokens.is_empty())
				continue;

			// Simple 1 line C style object-like macros
			if (line_tokens[0] == String("define")) {
				String define_name = line_tokens[1];
				String define_value;
				for (int j = 2; j < line_tokens.size(); ++j) {
					define_value += line_tokens[j];
				}

				macro_defines[define_name] = define_value;
				macro_define_keys = macro_defines.keys();

				continue;
			}

			// Replace defines with their value, this has to be done before any other tokens are processed so the text will be correct.
			if (macro_define_keys.size() > 0) {
				for (int j = 0; j < line_tokens.size(); ++j) {
					for (int k = 0; k < macro_define_keys.size(); ++k) {
						if (line_tokens[j].contains(macro_define_keys[k]) and !line_tokens[j].contains(macro_defines[macro_define_keys[k]])) {
							line_tokens[j] = line_tokens[j].replace(macro_define_keys[k], macro_defines[macro_define_keys[k]]);
						}
					}
				}
			}

			// Replace 'comptime var X = Y' with 'set_var("X", "Y")', evaluate the result, and save it in the comptime variables map.
			if (line_tokens[0] == String("comptime") and line_tokens.size() >= 5) {
				comptime_variables[line_tokens[2]] = comptime_execute(get_comptime_var_value(line_tokens));
				continue;
			}

			if (is_inside_condition and !conditional_stack.front()) {
				if (line_tokens[0] != "end" and conditional_stack.size() > 0)
					continue;
			}

			// If-then-else statements
			if (line_tokens[0] == String("if")) {
				String if_condition = "";

				String expr = String().join(expression_tokens);
				if (!expr.is_empty()) {
					compiled_expressions.append(expr.trim_suffix("+"));
					expression_tokens.clear();
				}

				for (int j = 1; j < line_tokens.size(); ++j) {
					if (!line_tokens.size() - 1) {
						if_condition += line_tokens[j] + " ";
					} else {
						if_condition += line_tokens[j];
					}
				}

				is_inside_condition = true;
				conditional_stack.push_front(comptime_execute(check_for_comptime_vars(if_condition)));
				continue;
			}

			if (line_tokens[0] == String("end")) {
				is_inside_condition = false;
				conditional_stack.pop_front();

				String expr = String().join(expression_tokens);
				if (!expr.is_empty()) {
					compiled_expressions.append(expr.trim_suffix("+"));
					expression_tokens.clear();
				}

				continue;
			}

			// Includes that works sorta like the C #include, it just compiles and then pastes the expressions right in.
			if (line_tokens[0] == String("include")) {
				String file_name;
				for (int j = 1; j < line_tokens.size(); ++j) {
					file_name += line_tokens[j];
				}

				// Check if the file is already being included to prevent infinite recursion due to circular imports.
				ERR_FAIL_COND_V_MSG(current_includes.find(file_name) != current_includes.end(), PackedStringArray(),
						vformat("GDExpr circular include detected in %s...aborting include.", file_name));

				current_includes.insert(file_name);
				String file_content = parse_include_file(file_name);
				PackedStringArray include_file_tokens = compile(file_content);

				for (int i = 0; i < include_file_tokens.size(); ++i) {
					compiled_expressions.append(include_file_tokens[i]);
				}

				current_includes.erase(file_name);
				continue;
			}

			// Exit the program and return all the previously compiled expressions.
			if (line_tokens[0] == String("exit")) {
				break;
			}

			// Exit the program and returns nothing.
			if (line_tokens[0] == String("bail")) {
				return PackedStringArray();
			}

			// Replace 'var X = Y' with 'set_var("X", "Y")'
			if (line_tokens[0] == String("var")) {
				preprocess_variables(line_tokens, expression_tokens);
				continue;
			}

			// Repeat statement
			// TODO - Rewrite this, it is awful.
			if (line_tokens[0] == String("repeat")) {
				bool is_adding_expressions = false;
				PackedStringArray original_expression_tokens;
				if (line_tokens.size() == 2) {
					String expr = String().join(expression_tokens);
					if (line_tokens[1].begins_with("+")) {
						is_adding_expressions = true;
						line_tokens[1] = line_tokens[1].trim_prefix("+");
						expression_tokens.insert(0, "(");
						expression_tokens.append(")");
						expression_tokens.append("+");
						original_expression_tokens = expression_tokens;
					}

					String iterations_token = line_tokens[1];
					int64_t repeat_num = 0;

					if (iterations_token.is_valid_int()) {
						repeat_num = iterations_token.to_int();
					} else {
						if (comptime_variables.has(iterations_token)) {
							Variant variable_value = comptime_variables[iterations_token];
							if (variable_value.get_type() == Variant::INT) {
								repeat_num = variable_value;
							}
						}
					}

					repeat_num -= 1; // why is this needed?! it breaks without it but I don't get why???
					if (is_adding_expressions) // oh jeez
						repeat_num -= 1; // wtf

					if (!expr.is_empty()) {
						while (repeat_num >= 0) {
							if (!is_adding_expressions) {
								compiled_expressions.append(expr.trim_suffix("+"));
							} else {
								expression_tokens.append_array(original_expression_tokens);
							}
							repeat_num--;
						}

						if (is_adding_expressions)
							compiled_expressions.append(String().join(expression_tokens).trim_suffix("+"));
					}

					expression_tokens.clear();
				}
				continue;
			}

			// Break expression
			if (line_tokens[0] == String("break") or line_tokens[0] == String("---")) {
				String expr = String().join(expression_tokens);
				if (!expr.is_empty()) {
					compiled_expressions.append(expr.trim_suffix("+"));
					expression_tokens.clear();
					continue;
				}
			}

			// End of multi-line declarations
			if (line_tokens[0] == ")" and is_inside_multiline_declaration) {
				String var_token = vformat("set_var(\"%s\", %s)", current_variable_name, current_variable_value) + ")+";
				var_token = check_for_comptime_vars(var_token);

				is_inside_multiline_declaration = false;
				expression_tokens.append(var_token);
				String expr = String().join(expression_tokens);
				if (!expr.is_empty()) {
					compiled_expressions.append(expr.trim_suffix("+"));
					expression_tokens.clear();
					continue;
				}
			}

			// No need to iterate over tokens if there are no variables.
			if (variables.size() < 1) {
				expression_tokens.append(String().join(line_tokens));
				continue;
			}

			for (int j = 0; j < line_tokens.size(); ++j) {
				// Replace token "X" with a variable name declared using "var"
				for (String variable : variables) {
					String current_variable = line_tokens[j];
					if (current_variable.contains(variable) and !current_variable.contains("get_var(\"")) {
						line_tokens[j] = current_variable.replace(variable, vformat("get_var(\"%s\")", variable));
					}
				}

				line_tokens[j] = check_for_comptime_vars(line_tokens[j]);

				// Add processed tokens
				if (line_tokens[j] != "" and !is_inside_multiline_declaration)
					expression_tokens.append(line_tokens[j]);
			}

			// If inside multi-line declaration, add text to the variable value.
			if (is_inside_multiline_declaration) {
				current_variable_value += String().join(line_tokens);
				continue;
			}
		}

		if (!expression_tokens.is_empty())
			compiled_expressions.append(String().join(expression_tokens).trim_suffix("+"));
		return compiled_expressions;
	}

	// TODO - This should sanitize gdexpr files at compile time to prevent certain potentially malicious functions from being run by end users.
	// The compiler should emit a warning and remove the expression where the potentially malicious function is found so it never gets executed.
	// Assuming all the functions passed in with base_instance are also properly sanitized
	// this should make gdexpr a safe sandboxed environment for modding or other user generated content which can't crash the application and can't call functions that might
	// harm other users. Im thinking this function should sanitize all the potentially malicious functions in @GlobalScope and the Object class (if the user passes in a
	// base_instance it will have to be an Object) Then this function will also take a function pointer that can be passed in from the caller to sanitize additional functions
	// that the compiler doesn't handle.
	//void sanitize() {}

	PackedStringArray compile_directory(String dir_path) { return compile(parse_directory(dir_path)); }

	PackedStringArray compile_file(String file_path) {
#ifdef GDEXPR_COMPILER_DEBUG
		PackedStringArray arr = compile(parse_file(file_path));
		UtilityFunctions::print("GDExpr compiled expressions: ", arr);
		return arr;
#else
		return compile(parse_file(file_path));
#endif
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("static_compile", "user_expression_inputs", "base_expression_instance", "user_file_to_compile"), &GDExpr::static_compile);
		ClassDB::bind_method(D_METHOD("execute_precompiled_expressions", "compiled_expression", "string_to_execute"), &GDExpr::execute_precompiled_expressions);
		ClassDB::bind_method(D_METHOD("execute", "user_expression_inputs", "base_expression_instance", "string_to_execute"), &GDExpr::execute);
		ClassDB::bind_method(D_METHOD("execute_file", "user_expression_inputs", "base_expression_instance", "user_file_to_compile"), &GDExpr::execute_file);
		ClassDB::bind_method(D_METHOD("execute_directory", "user_expression_inputs", "base_expression_instance", "user_dir_to_compile"), &GDExpr::execute_directory);

		ClassDB::bind_method(D_METHOD("sort_by_longest", "a", "b"), &GDExpr::sort_by_longest);
	}

public:
	// Don't use this. It has to be public because it needs to be called from inside expressions.
	bool sort_by_longest(const String &a, const String &b) const { return a.length() > b.length(); }

	// Statically compile a gdexpr file to a sequence of godot expressions but does not execute them. This works like a static compiler.
	// If your script does not need runtime data you should always statically compile as it will completely eliminate the compile time cost when running the expressions at
	// runtime. Returns a PackedStringArray of godot expressions that are ready for execution with the execute_precompiled_expressions function.
	PackedStringArray static_compile(Array user_expression_inputs, Ref<GDExprScript> base_expression_instance, String user_file_to_compile) {
		base_instance = base_expression_instance;
		expression_inputs = user_expression_inputs;
		file_to_compile = user_file_to_compile;

		return compile_file(file_to_compile);
	}

	// TODO - create some kind of algorithm that is able to report gdexpr files that have changed since the last compile so users can write code that allows pre compiling and
	// also hot reloading. Not sure if this should actually be in the compiler or in a seperate interface though...it should be possible to do it outside of the compiler.
	// NOTE: I actually already did this in python in the check_mod_for changes function of game_objects.py found here:
	// https://github.com/dementive/JominiTools/blob/64b6b9da14fe766b7d881e56b903c7c01264e853/src/game_objects.py#L25
	// The algorithm should work the same, just check os.stat of each compiled file, cache the value, and then have a "script_changed" signal send from somewhere that will
	// recheck os.stat to see if the file was changed. This would probably require some kind of cache to store the os.stat values to be implemented too, that is how
	// JominiTools pulls it off.

	// Execute a sequence of expressions that were precompiled with the static_compile function.
	// Returns the results of each expression executed in an Array.
	Array execute_precompiled_expressions(PackedStringArray compiled_expression, String string_to_execute) {
		return _execute_expressions(compiled_expression, file_to_compile, true);
	}

	// Compiles a String with GDExpr code in it to a sequence of godot expressions and execute them.
	// Returns the results of each expression executed in an Array.
	Array execute(Array user_expression_inputs, Ref<GDExprScript> base_expression_instance, String string_to_execute) {
		base_instance = base_expression_instance;
		expression_inputs = user_expression_inputs;
#ifdef GDEXPR_COMPILER_TIMING_DEBUG
		TIME_MICRO(compile, PackedStringArray compiled_expression = compile(string_to_execute))
		return _execute_expressions(compiled_expression, file_to_compile, true);
#else
		PackedStringArray compiled_expression = compile(file_to_compile);
		return _execute_expressions(compiled_expression, file_to_compile, true);
#endif
	}

	// Compile a gdexpr file to a sequence of godot expressions and execute them.
	// Returns the results of each expression executed in an Array.
	Array execute_file(Array user_expression_inputs, Ref<GDExprScript> base_expression_instance, String user_file_to_compile) {
		base_instance = base_expression_instance;
		expression_inputs = user_expression_inputs;
		file_to_compile = user_file_to_compile;
#ifdef GDEXPR_COMPILER_TIMING_DEBUG
		TIME_MICRO(compile, PackedStringArray compiled_expression = compile_file(file_to_compile))
		return _execute_expressions(compiled_expression, file_to_compile, false);
#else
		PackedStringArray compiled_expression = compile_file(file_to_compile);
		return _execute_expressions(compiled_expression, file_to_compile, false);
#endif
	}

	// Search a directory for gdexpr files to compile then execute them.
	// This will process each gdexpr file in the directory
	// So be careful not to put gdexpr files that are only meant to be used as includes in the execution directory when using this function.
	// Returns the results of each expression executed in an Array.
	Array execute_directory(Array user_expression_inputs, Ref<GDExprScript> base_expression_instance, String user_dir_to_compile) {
		base_instance = base_expression_instance;
		expression_inputs = user_expression_inputs;
		file_to_compile = user_dir_to_compile;
#ifdef GDEXPR_COMPILER_TIMING_DEBUG
		TIME_MICRO(compile_directory, PackedStringArray compiled_expression = compile_directory(file_to_compile))
		return _execute_expressions(compiled_expression, file_to_compile, false);
#else
		PackedStringArray compiled_expression = compile_directory(file_to_compile);
		return _execute_expressions(compiled_expression, file_to_compile, false);
#endif
	}
};

} //namespace gdexpr

#endif // GDExpr_H
