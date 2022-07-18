# _app_ directory

This directory contains a copy of _rsc/main.cpp_ that you can use for 
your application if you prefer not to change _main.cpp_. It has been
modified so that its executable (_rscapp_) only includes the namespaces
`NodeBase` and `NodeTools`. To include additional namespaces, enable
the relevant `//&` comments and enable the related `#` comments in the
[CMakeLists](CMakeLists.txt) file. Its `main` also disables file
output, so you will probably want to delete that line of code.
