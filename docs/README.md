# Documentation

## RSC Foundation

These documents describe RSC's primary capabilities.

Document | Description
-------- | -----------
[RCS chapter 2](/docs/RCS-chapter-2.pdf) | chapter 2 of _Robust Communications Software_
[RCS tutorial](/docs/RCS-tutorial.pdf) | overview of _Robust Communications Software_
[RSC Product Overview](/docs/RSC-Product-Overview.pdf) | roadmap for the Robust Services Core
[Software Techniques for Lemmings](https://www.codeproject.com/Articles/5258540/Software-Techniques-for-Lemmings) | article on popular but dubious techniques and RSC's alternatives
[Robust C++ : P and V Considered Harmful](https://www.codeproject.com/Articles/5246597/Robust-Cplusplus-P-and-V-Considered-Harmful) | article on cooperative/proportional scheduling
[Robust C++ : Safety Net](https://www.codeproject.com/Articles/5165710/Robust-Cplusplus-Safety-Net) | article on recovering from exceptions
[Robust C++ : Object Pools](https://www.codeproject.com/Articles/5166096/Robust-Cplusplus-Object-Pools) | article on recovering from memory leaks
[Robust C++ : Initialization and Restarts](https://www.codeproject.com/Articles/5254138/Robust-Cplusplus-Initialization-and-Restarts) | article on structuring `main` and recovering from memory corruption
[Debugging Live Systems](https://www.codeproject.com/Articles/5255828/Debugging-Live-Systems) | article on debugging capabilities
[A Command Line Interface (CLI) Framework](https://www.codeproject.com/Articles/5269493/A-Command-Line-Interface-CLI-Framework) | article on the command line interface
[Robust C++: Operational Aspects](https://www.codeproject.com/Articles/5274153/Robust-Cplusplus-Operational-Aspects) | article on configuration parameters, statistics, logs, and alarms
[A Template for Polymorphs](https://www.codeproject.com/Articles/5271143/A-Template-for-Polymorphs) | article on the `Registry` template
[Robust C++: Queue Templates](https://www.codeproject.com/Articles/5271081/Robust-Cplusplus-Queue-Templates) | article on the `Q1Way` and `Q2Way` templates
[Robust C++: Singletons](https://www.codeproject.com/Articles/5286932/Robust-Cplusplus-Singletons) | article on the `Singleton` template
[A Wrapper for `std::vector`](https://www.codeproject.com/Tips/5271013/A-Wrapper-for-std-vector) | article on the `Array` template
[RSC Software Design](/docs/RSC-Software-Design.pdf) | high-level design notes for various enhancements
[_output_](/output) directory | mostly output from tests, but also a summary of all [CLI commands](/output/help.cli.txt)
[RSC Windows Build Options](/docs/RSC-Windows-Build-Options.md) | build options used in VS2017
[RSC Coding Guidelines](/docs/RSC-Coding-Guidelines.md) | C++ coding guidelines

## C++ Static Analysis

These documents describe the C++ static analysis tools found in the _ct_ directory.

Document | Description
-------- | -----------
[RSC C++ Static Analysis Tools](/docs/RSC-Cpp-Static-Analysis-Tools.md) | overview of the C++ static analysis tools
[RSC C++11 Exclusions](/docs/RSC-Cpp11-Exclusions.md) | C++11 language features not supported by the static analysis tools
[A Static Analysis Tool for C++](https://www.codeproject.com/Articles/5246833/A-Static-Analysis-Tool-for-Cplusplus) | article on the static analysis tools

## RSC Session Processing and Applications

These documents describe the session processing framework provided with RSC and the POTS and
Diplomacy bot applications that use it.

Document | Description
-------- | -----------
[RSC Session Processing tutorial](/docs/RSC-Session-Processing-tutorial.pdf) | overview of `SessionBase`
[RSC Session Processing](/docs/RSC-Session-Processing.pdf) | design guide for `SessionBase` applications
[RSC POTS Application](/docs/RSC-POTS-Application.md) | overview of the POTS application
[_A Pattern Language of Call Processing_](/docs/PLCP.pdf) | patterns used in session processing
[RSC Diplomacy Bot](/docs/RSC-Diplomacy.md) | overview of the Diplomacy Bot application
