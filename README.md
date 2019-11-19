# Jeff's VM Language (v2.0)
A toy virtual machine made from scratch which supports an object-oriented, high-order programming language with built-in libraries for I/O, basic networking, and multi-threading. 

Example hello world program (hello.jl):

    import io
    io.println('Hello, world!')

To run the program hello.jl:

    jlc hello.jl

To output the instructionized version of the program:

    jlc -ex=false -m hello.jl

The output will be saved to hello.jm:

    rmdl  io
    mdst  io
    push  io
    res   'Hello, world!'
    call  println
    exit  0
