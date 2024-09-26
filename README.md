# GDExpr - Scripting Language for Godot

GDExpr is a structured dynamically typed scripting language with tight integration into the Godot engine that compiles to a sequence of Godot expressions and executes them.

### Example Usage
Executing a gdexpr file with GDscript is as simple as creating a script such as this:

```gdscript
# Test GDExpr script.
# This is an example of how to create your own GDExpr classes in gdscript.
# All functions in this class can be called during the GDExpr runtime.

class_name ExampleGDExprScript
extends BaseGDExprScript  # Note that you must inherit from BaseGDExprScript.


func run() -> void:
    # Just like regular godot expressions, an Array of input variables can be passed directly into gdexpr.
    var expression_inputs: Array = Array()

    var script_results: Array = GDExpr.execute_file(
        expression_inputs, self, "res://demo/test.gdexpr"
    )
    for i in script_results:
        print("Result: ", i)


# All functions declared in a gdexpr script will be available in gdexpr at runtime. This hello_gdexpr() function can be called directly in your gdexpr script.
func hello_gdexpr() -> String:
    return "Hello World"

```

Then somewhere else in any gdscript you can execute the gdexpr script like this:
```gdscript
# Call the example gdexpr script
func _ready() -> void:
    var example_script_context = preload("res://demo/example_gdexpr_script.gd").new()
    example_script_context.run()

```

This is what the `test.gdexpr` GDExpr file would look like to print Hello World:

```gdscript
hello_gdexpr() # Call `hello_gdexpr` on the script object instance.
```

### Installing

Installing from the godot asset marketplace is the easiest way:

https://godotengine.org/asset-library/asset/3306

### What is GDExpr?

GDExpr is a dynamically typed scripting language that compiles to a sequence of Godot expressions and executes them.
It is written as a C++ GDExtension, in a single header file, in under 1000 lines of code that only uses godot-cpp headers, with no standard library usage. It is written as a Godot add-on so can be used from either C++ (or any other GDExtension language) and GDScript.

GDExpr can be run as a JIT compiler to compile and execute code at runtime, so it allows for hot reloading and quick runtime testing. It can be used to statically compile GDExpr code to Godot expressions which can then be executed at runtime without incurring the compile time overhead.

GDExpr is a [structured](https://en.wikipedia.org/wiki/Structured_program_theorem) language and doesn't even have any non-structural procedures (such as function declarations or goto statements). This means the control flow of any GDExpr program will always be a straight line from the top of the script to the bottom.

Unlike most sane languages that compile to byte code or machine code GDExpr instead compiles to Godot expressions. There are a lot of advantages to this approach, but there are also a few downsides.

The killer feature of Godot expressions that made me want to compile to them is their ability to call any @GlobalScope functions or any function passed in on a object pointer.


All you have to do is write 1 line of code to bind the function in C++. In GDscript you don't have to write any bindings at all, the functions bind automatically!
This makes your C++ code considerably simpler than most other scripting solutions would...
Compare this to creating function bindings from C->Lua: https://chsasank.com/lua-c-wrapping.html


Additionally Godot expressions natively support every Variant type and all operations on them out of the box...which means you can do pretty much anything with any type.


They also fully implement conditional logic and pretty much all the ways you can use it.

All of the features Godot expressions have don't have to be implemented in GDExpr at all since godot expressions are able to handle them.
This makes the language implementation a lot more concise in addition to, having a simple API, and gdexpr syntax is also easy to use.

Another great feature of Godot expressions have is they will pretty much never cause crashes no matter how stupid the input you throw at it is, and sometimes it even
gives sensible error messages! In my entire time writing this there hasn't been any input I could throw at it that caused a crash.

Since the compiler also manages the runtime if the user makes syntax errors, the compiler will notify what expressions failed, the file it failed in, and the error message the Godot expression compiler emitted.

To see everything you can possibly do with GDExpr see the `test.gdexpr` file in the demo scene of this repository (and read the documentation...of course).

### Why use GDExpr?

This is why I made GDExpr and what I plan to use it for.

GDExpr is not meant to be the main language you use to develop your game, it is designed to be an auxiliary scripting language.

Modern games are often developed using 2 different programming languages. A low level language to implement all the functionality and a much simpler scripting language to define the functionality.  Often during development tasks are delegated to the scripting language, and can be assigned to Content Designers rather than Programmers. It also makes the parts of your game implemented with the scripting language significantly easier to work since the scripts are generally much simpler than your actual game code. Tim Cain can explain it much better than I can: https://www.youtube.com/watch?v=ljnaL7N5qtw

In Godot the main scripting language is GDscript. I am writing my game entirely with GDExtension C++, using GDscript as the main scripting language, in this context feels awkward, due to the way Godot scripts must be tied to a node in the scene tree.

So what are my exact requirements for a scripting language?

In my mind the ideal scripting language would have all of these things:

1. Simplicity - Content designers should be able to fully understand all the language concepts with ease. It should also be fast to write because it is simple.

2. Dynamic Typing - Having to worry about statically typing variables isn't something you should have to do when using a scripting language.

3. Ability to stay DRY (Don't Repeat Yourself) - Simple config languages like JSON and YAML make it nearly impossible to not repeat yourself. An actual scripting language should have multiple ways to avoid code repetition, GDExpr does this with loops, C style macros, and C style includes. Unlike most languages, you cannot declare functions in GDExpr, because functions obfuscate the control flow.

4. Dynamic Configuration - All static config file formats are awful. This blog post explains exactly how I feel about this: https://beepb00p.xyz/configs-suck.html

5. Be Sandboxable - It should be possible to run the scripting language in a sandboxed environment, so only the functions you expose to the environment are possible to use. This enables you to use GDExpr as a scripting language for modding or other user generated content which should not crash the application and call functions that might harm other users.

6. Easily bindable to C++ functions - If I purely wanted performance no doubt by far the best scripting solution would be LuaJIT...however binding Lua functions to C++ would make my game code that is already very complex even more complex, it also isn't trivial to integrate in a cross platform way into the SCons build system, and it has 1 based indexing. It turns out that binding C++ functions to pretty much any programming language requires tons of boilerplate. I also looked into what other people were trying to bind to GDExtension and tried to use Julia and Angelscript to make bindings for. In contrast to every other solution I could find, GDExpr bindings are just 1 line of code when used with C++ and 0 lines of code when using with gdscript, making bindings trivial. Even binding functions to and parsing something extremely simple like JSON is more complex than GDExpr bindings since GDExpr is able to execute the functions directly on an object pointer.

7. Hot Reloading - GDExpr works just like JIT compiler but also exposed an API that can statically compile GDexpr files. 

8. Good Debugging - The scripting language should be simple enough to allow Content Developers to debug most problems without help.

9. Safe - It should be impossible for a scripting language to crash the application. GDExpr is designed to never crash, no matter what inputs you give it, the only way it can possibly crash is if you send inputs that your functions do not know how to handle.

10. Fast Execution - GDExpr should be fast. it's difficult to benchmark how fast GDExpr is. There is more info on performance later.

GDExpr fulfills all of these requirements.

### Documentation

For a full overview of the GDExpr syntax and usage see the documentation: https://github.com/dementive/gdexpr/wiki

### Performance

So how fast is GDExpr? Executing single expressions takes only a few microseconds and is comparable to GDscript performance, but it is slower than GDscript.

For me this is way faster than I will ever need it to be, being able to make millions of function calls in milliseconds will likely be more than enough for your application too. So I think the benefits gained from the convenience and ease of use greatly outweighs the performance hit.

Additionally if you write mostly comptime code the runtime cost can be greatly reduced if you also precompile.

### Godot expression limitations
Compiling to Godot expressions has a lot of fun things to work around that a normal compiler would never have to worry about.
For example, the only possible way to implement if statements and loops was by doing it at compile time and adding a "comptime" keyword that works kind of like [Zig comptime](https://ziglang.org/documentation/0.13.0/#comptime).
I would really prefer to not have to implement comptime because it is a complex concept for scripting language syntax but there is actually no other possible way to do ifs or loops that I could think of.


Another limitation is that any function that returns void in the runtime will kill the execution of the current expression, preventing other subsequent statements from being run.
This isn't something the compiler can fix either, it can help mitigate some of the side effects of it, but the user still has to make their functions return valid Variants.

### Contributing

GDExpr uses the MIT license and contributions are highly encouraged.

Feel free to open up issues on github if you want any new features or find bugs. I don't want to add a ton of new features to the language in my opinion it is mostly complete but any suggestions are still welcome.

There are a few things marked as TODO, see here: https://github.com/dementive/gdexpr/wiki/TODO

Before making PR's, open up an issue to discuss your changes, this project is very opinionated. So be sure to discuss potential changes first to make sure they align with the projects goals.

### Environment Setup

First you will need to install [pre-commit](https://pre-commit.com/). I am using pre-commit for a lot of things, it currently lints and formats all of the C++, GDScript, and Scons (python) files. This ensures that all code committed is consistently styled and it most likely still works if it passed the lints. This way I never have to worry about how I'm writing my code I just let the formatter do it for me and it's also impossible to commit code that breaks the linters.

To install pre-commit on your system just run `sudo pacman -S pre-commit` (or whatever it is in your package manager).


Then clone the repository:
```bash
git clone https://github.com/dementive/gdexpr
cd gdexpr
```

Once your in the gdexpr directory install the pre-commit hooks to your local repo with: `pre-commit install`
You can test if the hooks were correctly installed by running: `pre-commit run --all-files`

### Compiling

Before compiling you'll need to initialize the submodules using: `git submodule update --init --recursive`

To compile use this command:
`scons target=template_debug debug_symbols=yes dev_build=yes optimize=debug symbols_visibility=visible`

This will compile in debug mode and with debug symbols that are actually readable.

### C++ Coding Guidelines

The coding guidelines are the exact same as the godot engine guidelines, because the code is the same.

https://docs.godotengine.org/en/stable/contributing/development/cpp_usage_guidelines.html

Basically this means no STL, no auto keyword, and no lambdas. Since GDExtension is basically an extension of the game engine it makes sense to follow the same style unless it's absolutely necessary.

Reading through this:

https://docs.godotengine.org/en/stable/contributing/development/

And then this:

https://docs.godotengine.org/en/stable/contributing/development/core_and_modules


Should cover everything you need to know when it comes to actually using GDExtension.

### Coding style

Code style should be the exact same as the godot engine code

Although most of the formatting stuff you won't need to worry about if you setup the pre-commit hooks.

https://docs.godotengine.org/en/stable/contributing/development/code_style_guidelines.html


Make sure to setup the clangd LSP in your editor. Setting up clangd is very important, clangd is awesome, I love clangd, you should use clangd!


### License

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
