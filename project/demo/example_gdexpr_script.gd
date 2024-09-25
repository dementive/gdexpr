# Test GDExpr script.
# This is an example of how to create your own GDExpr classes in gdscript.
# All functions in this class can be called during the GDExpr runtime.

class_name ExampleGDExprScript
extends BaseGDExprScript  # Note that you must inherit from BaseGDExprScript.


func run() -> void:
	var expression_inputs: Array = Array()

	var script_results: Array = GDExpr.execute_file(
		expression_inputs, self, "res://demo/test.gdexpr"
	)
	for i in script_results:
		print("Result: ", i)


func run_config() -> void:
	var expression_inputs: Array = Array()

	var script_results: Array = GDExpr.execute_file(
		expression_inputs, self, "res://demo/test_config.gdexpr"
	)
	for i in script_results:
		print("Result: ", i)


# NOTE: Functions run in GDExpr cannot return void, they must return a Variant value of some kind.
# If they return void when evaluated as an expression
# they will return Variant::NIL, which will cause the expression to fail.
# So always just return an int or some kind of number.
# All functions declared in a gdexpr script will be available in gdexpr at runtime.
func do_thing() -> int:
	return 1


func do_thing_with_variable(variable: int) -> int:
	return variable * 5000


func do_thing_with_dictionary(dict_variable: Dictionary) -> int:
	return dict_variable.has("test_key")


func do_thing_with_array(array_variable: Array) -> int:
	return array_variable.size() == 5
