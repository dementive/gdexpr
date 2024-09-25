extends Node


# Call the example gdexpr script
func _ready() -> void:
	var example_script_context = preload("res://demo/example_gdexpr_script.gd").new()
	example_script_context.run_config()
	example_script_context.run()
