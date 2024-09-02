# Test GDExpr script.
# This is an example of how to create your own GDExpr classes in gdscript with functions that can be called during the GDExpr runtime.

extends BaseGDExprScript  # Note that you must inherit from BaseGDExprScript.
class_name ExampleGDExprScript


# NOTE: Functions run in GDExpr cannot return void, they must return a Variant value of some kind.
# If they return void when evaluated as an expression they will return Variant::NIL, which will cause the expression to fail.
# So always just return an int or some kind of number.
# All functions declared in a gdexpr script will be available in gdexpr at runtime.
func do_thing() -> int:
	print("Hello GDScript!")
	return 1


func do_thing_with_variable(variable: int) -> int:
	return variable * 5000


func do_thing_with_dictionary(dict_variable: Dictionary) -> int:
	return dict_variable.has("test_key")


func do_thing_with_array(array_variable: Array) -> int:
	return array_variable.size() == 5
