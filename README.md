# A Bytecode Interpreter for Secure Program Execution in Untrusted Main Memory

Bispe is a bytecode interpreter which protects executed programs against physical attacks on memory such as Coldboot or DMA attacks.
To this end, the interpreter encrypts all code and data of executed programs, holding them in the clear only in CPU registers. 
As a secure implementation of this idea requires deep system access, Bispe is devised as a Linux kernel module. 
Bispe is built on [TRESOR](https://www1.cs.fau.de/tresor), a memory attack resistant implementation of AES which is used to setup secure full disk encryption. In particular, TRESOR holds the used encryption key in the debug registers, i.e. not in vulnerable RAM.

This project originated from my bachelor's thesis at FAU Erlangen-Nuremberg, Germany.
The associated research paper was published at ESORICS 2015. 

Project homepage at the chair for IT Security Infrastructures at FAU:

https://www1.cs.fau.de/bispe

## Usage

### Preliminaries:
Your CPU needs support for the AVX and AES-NI instruction set extensions in order to run the interpreter (e.g a Intel Core i5/i7 CPU)
You can check this using:
```
grep -o -e avx -e aes /proc/cpuinfo
```

### Compilation:
Run `make` from the project's root directory. 
To compile the code, you will need `linux-headers-generic` and a recent gcc version. 

### Loading the kernel module:
```
cd bin
sudo insmod bispe_km.ko
```
You can verify if the kernel module was correctly loaded by looking at the output of 
```
dmesg | less
```

### Setting a password:
In order to compile/run encrypted programs, an encryption key is needed within the CPU's registers.
This key is derived from a user-specified password. 

Usually, to set the key in a manner secure against memory attacks, this would have to happen at boot time. 
This can be done by using the [TRESOR](https://www1.cs.fau.de/tresor) kernel patch. 
The patch also implements further measures to keep the key secret at system runtime. 

To just play around with the interpreter, you can set a password with the interpreter frontend
```
sudo ./bispe -p
```
(refer to 'Implementation notes' to see why this and all subsequent commands need sudo)

### Compiling to encrypted bytecode:
The interpreter uses encrypted bytecode as input. 
The accompanying compiler produces those from code files.

To compile a file and encrypt it using the currently set encryption key:
```
sudo ./compiler ../examples/hello_world.scll
```

### Running the interpreter:
Finally, the encrypted bytecode is passed to the interpreter frontend for execution:
```
sudo ./bispe ../examples/hello_world.scle 1 2
```
Here, `1 2` refers to the program's arguments. 
In this case, the program just adds them, that's why it outputs `3`.

### Unloading the kernel module:
If you want to unload the kernel module, use: 
```
sudo rmmod bispe_km
```

## Research paper

The research paper was published in: 

> Maximilian Seitzer, Michael Gruhn, and Tilo Müller. *A Bytecode Interpreter for Secure Program Execution in Untrusted Main Memory*. In Computer Security – ESORICS 2015, volume 9327 of Lecture Notes in Computer Science, pages 376–395. Springer International Publishing, 2015.

Paper abstract: 
> Physical access to a system allows attackers to read out RAM through cold boot and DMA attacks. Thus far, counter measures protect only against attacks targeting disk encryption keys, while the remaining memory content is left vulnerable. We present a bytecode interpreter that protects code and data of programs against memory attacks by executing them without using RAM for sensitive content. Any program content within memory is encrypted, for which the interpreter utilizes TRESOR, a cold boot resistant implementation of the AES cipher. The interpreter was developed as a Linux kernel module, taking advantage of the CPU instruction sets AVX for additional registers, and AES-NI for fast encryption. We show that the interpreter is secure against memory attacks, and that the overall performance is only a factor of 4 times slower than the performance of Python. Moreover, the performance penalty is mostly induced by the encryption.

## Implementation notes

As this is a research prototype, the amount of programs the interpreter can execute are limited. So far, only a small language with C-like syntax is supported. 
See the examples folder for a couple of programs in the interpreter's syntax. 
The grammar the compiler accepts can be found in `compiler/grammar.txt`.

I implemented the compiler before taking any compiler classes. 
As such, the compiler's code might look sketchy here and there, but it gets the job done. 
The focus of the project is not the compiler anyways. 
And its error messages are not very helpful :-)

### Why does it need sudo?
The compiler and the interpreter frontend communicate with the interpreter kernel module over the kernel's sys-filesystem. 
A change to the Linux kernel that was made after the project's completion introduced that only root can write to objects within the sys-filesystem. 
This is the reason sudo is needed.
To change that, an alternative kernel communication mechanism would have to be used.
