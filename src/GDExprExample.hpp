#ifndef GDExprTest_H
#define GDExprTest_H

#include "GDExpr.hpp"

using namespace godot;

namespace gdexpr {

// Test GDExpr script.
// This is an example of how to create your own GDExpr classes in C++ with functions that can be called during the GDExpr runtime.
// Note that you must inherit from BaseGDExprScript.
class GDExprExampleScript : public BaseGDExprScript {
	GDCLASS(GDExprExampleScript, BaseGDExprScript)

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("do_thing"), &GDExprExampleScript::do_thing);
		ClassDB::bind_method(D_METHOD("do_thing_with_variable", "variable"), &GDExprExampleScript::do_thing_with_variable);
		ClassDB::bind_method(D_METHOD("do_thing_with_dictionary", "dict_variable"), &GDExprExampleScript::do_thing_with_dictionary);
		ClassDB::bind_method(D_METHOD("do_thing_with_array", "array_variable"), &GDExprExampleScript::do_thing_with_array);
	}

public:
	GDExprExampleScript() {}

	// NOTE: Functions run in GDExpr cannot return void, they must return a Variant value of some kind.
	// If they return void when evaluated as an expression they will return Variant::NIL, which will cause the expression to fail.
	// So always just return an int or some kind of number.
	int do_thing() { return 999; }
	int do_thing_with_variable(int variable) { return variable * 5000; }
	int do_thing_with_dictionary(Dictionary dict_variable) { return dict_variable.has("test_key"); }
	int do_thing_with_array(Array array_variable) { return array_variable.size() == 5; }
};

// Node class that will run the gdexpr tests in the _ready function.
class GDExprExampleNode : public Node {
	GDCLASS(GDExprExampleNode, Node)

private:
	// This is an example of how you would execute a gdexpr file and print out the results of each expression executed.
	void test_gdexpr() {
		GET_SINGLETON(GDExpr, gdexpr_script)
		Array expression_inputs = Array();
		Ref<GDExprExampleScript> script_context = memnew(GDExprExampleScript);

		// GDExpr::execute_file returns an Array with the Variant results of every expression that was executed in the gdexpr file.
		Array script_results = gdexpr_script->execute_file(expression_inputs, script_context, "res://demo/test.gdexpr");
		for (int i = 0; i < script_results.size(); ++i) {
			UtilityFunctions::print("RESULT: ", script_results[i]);
		}
	}

protected:
	static void _bind_methods() {}

public:
	GDExprExampleNode() {}

	void _ready() override {
		test_gdexpr();
	}
};

}

#endif
