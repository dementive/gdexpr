extends Node

# Call the example gdexpr script
func _ready() -> void:
	return # remove when this script actually works...

	var expression_inputs: Array = Array()
	
	# TODO FIXME! - this doesn't work??? It does work in C++ though and is this:
	# Ref<GDExprExampleScript> script_context = memnew(GDExprExampleScript);
	# I thought using .new() would return the Ref but for some reason it passes it in as null in gdscript...
	var example_script_context = ExampleGDExprScript.new()

	var script_results: Array = GDExpr.execute_file(
		expression_inputs, example_script_context, "res://scripts/test.gdexpr"
	)
	for i in script_results:
		print("Result: ", i)
