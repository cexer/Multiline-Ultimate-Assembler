# Multiline Ultimate Assembler

Multiline Ultimate Assembler is a multiline (and ultimate) assembler (and
disassembler) plugin for [x64dbg](https://x64dbg.com/) and
[OllyDbg](http://www.ollydbg.de/). It's a perfect tool for modifying and
extending a compiled executable functionality, writing code caves, etc.

[🏠 Homepage](https://ramensoftware.com/multimate-assembler)

![Screenshot](screenshot.png)


## About this Fork

Automate code memory allocation. syntax:

````
<&label_name=size>
	mov ...
	jmp ...
````

Parameters:

* `label_name` must at least 4 alpha-number charactors.
* `=size` memory size to alloc. this is optional, default size is 100 bytes.


Related topics:

* (Add cheat engine style auto assembler for code injection)(https://github.com/x64dbg/x64dbg/issues/729)
* (Please add Cheat Engine like code cave management)[https://github.com/m417z/Multiline-Ultimate-Assembler/issues/1]