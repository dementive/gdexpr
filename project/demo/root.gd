extends Node


# Call the example gdexpr script
func _ready() -> void:
	var expression_inputs: Array = Array()
	var script_context = ExampleGDExprScript.new()

	var script_results: Array = GDExpr.execute_file(
		expression_inputs, script_context, "res://scripts/test.gdexpr"
	)
	for i in script_results:
		print("Result: ", i)
