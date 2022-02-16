# "Advanced Topics in Operating Systems" 
 M.Sc. course at the Reichman University <br/>
 Lecturer and guide - Dr. Oren Laadan <br/>
## Intro

This program is a part of our 3rd and last assignment. <br/>
Check out [os-hw3/assignment-3.md](https://github.com/GlaiChen/os-hw3/blob/main/assignment-3.md) for full details of the assignment. <br/>
The group part will also appear at our group repo at the following link: <br/>https://github.com/academy-dt/kvm-hello-world<br/>

## Invidual Part - Kernel Virtual Machine
In several lectures in the course, we have learned about virtualization from most of its aspects. <br/>
1. In Lecture #9, we've been shown by Dr. Laadan 2 mechanisms (you can find it [here](https://github.com/GlaiChen/os-hw3/blob/main/AOS-2022-L09.pdf), at slides 26 && 27). <br/>
(1.1) The first mechanism is of KVM as despicted in slide 26: <br/>
<img src="/images/slide_26.jpg"> <br/><br/>
First, it is Type II hypervisor,means it is a hosted hypervisor, which support guest virtual machines by coordinating calls for CPU, memory and other resources through the host's resources. In simple words -type II runs on top of an OS. <br/>
In our case study, the KVM has been written as linux kernel module and for linux and Intel VT-x purpose and relies on QEMU. <br/>
They actually virtualized only the CPU and/or MMU, and all of the rest of resources, such as BIOS, devices, etc, have been exported to the User Space and relied on an emulator project called QEMU and used this architecture emulator to make all of the "jobs" they did not want to make inside the Kernel Space. <br/> 
In fact, the architecture of that mechanism is based on 3 layers - User mode, Kernel mode and Non root, which is the guest. Both User and Kernel modes are rooted.
First thing first, the user mode process (QEMU) start with ending a syscall to the kernel module, which initate the guest enviorment (allocates memory, loads the BIOS, creates a device, etc),  and then eneters the guest context. <br/>
Once in the guest context, the BIOS start to run. As long as there's no traps, everything continues within the guest context. Once a trap occurs, the kernel module receives it. If it is something that the kernel "knows" how to handle by itself - it simply does that, and returns control back to the guest. Otherwise, the kernel module needs the QEMU's help. In that case, he returns the syscall to the user-root process with the return value holding all the information relevant for the qemu's work. The QEMU then does whatever he needs to do, and stores all the relevant data. It then triggers again the syscall, after completing everything it needed to do, and sending all relevant data. The Kernel module takes the QEMU's data (alongside with possibly things that it also needed to do) and gives back control to the guest. Basically, in this point the Kernel module and the QEMU did everything they were required by the non-root's trap (BIOS trap). This procedure carries on throughout all the guest execution time. <br/><br/>
(1.2) The second mechanism is of KVM as despicted in slide 27: <br/>
<img src="/images/slide_27.jpg"> <br/><br/>
So, once VMEXIT# occoured, we moved out from the guest mode (non root) to the user mode (root).
For this "movement", need to apply one of the exit conditions, which can be varry
2. When the OS runs out of memory it resorts to swapping pages to storage. Considering a system with QEMU/KVM hypervisor running several guests:
<br/>
   a. If the guest runs out of memory and starts swapping, the guest would like to call the disk. In that case, the hypervisor will receive that call, execute it and receive the answer. When it will be received, the hypervisor will move it back to the guest and the memory would be swaped. <br/>
   **To be Continued...**<br/>
   b. When the host runs out of they memory, and swaps out pages that are used by the hypervisor, <br/>
   **To be Continued...**<br/>
3. One difference between plain virtualization and nested virtualization is that the former can leverage EPT/NPT hardware extensions, while the latter cannot do so directly <br/>
   a. <br/>
    **To be Continued...**<br/>
   a. <br/>
    **To be Continued...**<br/>
4. The <a href="https://research.cs.wisc.edu/wind/Publications/antfarm-usenix06.pdf">Antfarm project</a> relies on shadow page-tables for introspection, to learn about processes inside the guest. It was built before the introduction of EPT/NPT. <br/>
 **To be Continued...**<br/>

<hr>

# Group programming
Made in collaboration with Daniel Trugman

## (1) KVM: hypervisor, guest(s), and hypercalls

### Question a.1

What is the size of the guest (physical) memory? How and where in the
code does the hypervisor allocate it? At what host (virtual) address is this
memory mapped?

### Answer a.1

As we can see in the `main` function, we call `vm_init` with `mem_size = 0x200000`. That means we allocate 2MB of memory space.
The actual allocation happens at `vm->mem = mmap(NULL, mem_size, ...)` where we ask the OS to give up a memory mapping of that size.
The `mmap` call returns the virtual address of this memory space from the host's perspective.

### Question a.2

Besides the guest memory, what additional memory is allocated? What is
stored in that memory? Where in the code is this memory allocated? At what
host/guest? (virtual/physical?) address is it located?

### Answer a.2

The `vcpu_init` method allocates an additional control block for the vCPU.
It first asks the KVM how big that structure is by calling `vcpu_mmap_size = ioctl(vm->sys_fd, KVM_GET_VCPU_MMAP_SIZE, 0)`,
and then allocates the memory at `vcpu->kvm_run = mmap(NULL, vcpu_mmap_size, ...)`.
Once again, the memory is allocated on the host, and the virtual address is stored in `vcpu->kvm_run`.
Later on, from inside `run_vm` we access this structure for information about the guest, such as the vmexit reason, IO info, etc.

The hypervisor then formats the guest memory and registers, to prepare for its
execution. (From here on, assume _"long"_ mode).

### Question a.3

The guest memory area is setup to contain the guest code, the guest page
table, and a stack. For each of these, identify where in the code it is setup,
and the address range it occupies (both guest-physical and host-virtual).

### Answer a.3

Inside `setup_long_mode` we can see the setup of the page tables.
We setup:
- PML4 at offset 0x2000
- PDPT at offset 0x3000
- PD at offset 0x4000
All relative to the beginning of the address space.
Meaning PML4 is at guest-physical address 0x2000 and host virtual address of `vm->mem + 0x2000`.

The stack is "allocated" at line 428: `regs.rsp = 2 << 20`.
Basically, `2 << 20` is the same value as `0x200000`, so we set the stack to be at the top of the address space.
In the guest-physical address, it will start at `0x200000` and grow down towards `0x0`.
In the host-virtual, it will start at `vm->mem + 0x200000` and grow down towards `vm->mem`.

The guest code segment is set up inside `setup_64bit_code_segment`.
We can see that the base of the segment is at address `0x0` (i.e `vm->mem` in host-virtual).
The actual copy of the copy into the guest's VM is at line 435: `memcpy(vm->mem, guest64, ...)`.
The guest code is exported as a extern byte array from the compile `guest.c` code.

```
0x000000 ---------------------------    --+
         Guest code                       |
0x001000 ---------------------------      |
         Nothing                          |
0x002000 ---------------------------      |
         PML4                             |
0x003000 ---------------------------      |
         PDPT                             |
0x004000 ---------------------------      +--- Single, 2MB page
         PD                               |
0x005000 ---------------------------      |
    .                                     |
    .      ^                              |
    .      |                              |
    .      |                              |
    .    STACK                            |
0x200000 ---------------------------    --+
```

### Question a.4

Examine the guest page table. How many levels does it have? How many
pages does it occupy? Describe the guest virtual-to-physical mappings: what
part(s) of the guest virtual address space is mapped, and to where?

### Answer a.4

The page table has 3 levels, each level occupies a single 4K memory region.
There is a single page mapped into the virtual address space, and that's a single 2M (PDE64_PS) page
that maps the entire memory space we allocated for this VM.

```
CR3 = 0x2000
         |
   +-----+
   |
   v
             PML4:
0x2000 ----> Entry | Where  | Bits
             ------+--------+----------------------------
             0     | 0x3000 | PRESENT, RW, USER
                        |
   +--------------------+
   |
   v
             PDPT:
0x3000 ----> Entry | Where  | Bits
             ------+--------+----------------------------
             0     | 0x4000 | PRESENT, RW, USER
                        |
   +--------------------+
   |
   v
             PDPT:
0x4000 ----> Entry | Where  | Bits
             ------+--------+----------------------------
             0     | 0x0    | PRESENT, RW, USER, PDE64_PS

```

For both (a.3) and (a.4), illustrate/visualize the hypervisor memory layout
and the guest page table structure and address translation. (Preferably in
text form).

### Question a.5

At what (guest virtual) address does the guest start execution? Where is
this address configured?

### Answer a.5

The guest starts execution at address 0, exactly where we placed our code.
We set the instruction pointer at line 426: `regs.rip = 0`.

Next, the hypervisor proceeds to run the guest. For simplicity, the guest code
(in _guest.c_) is very basic: one executable, a simple page table and a stack
(no switching). It prints "Hello, world!" using the `outb` instruction (writes
a byte to an IO port) which is a protected instruction that causes the guest to
exit to the hypervisor, which in turn prints the character.

(Notice how the `outb` instruction is embedded as assembly inside the guest's
C code; See [here](https://wiki.osdev.org/Inline_Assembly) for details on how
inline assembly works).

After the guest exits, the hypervisor regains control and checks the reason for
the exit to handle it accordingly.

### Question a.6

What port number does the guest use? How can the hypervisor know the port
number, and read the value written? Which memory buffer is used for this value?
How many exits occur during this print?

### Answer a.6

The guest uses port number `0xE9`.
This value is hardcoded into both applications, and when there's a vmexit, and the reason is `KVM_EXIT_IO` the host checks the conditions:
`if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xE9) {`
And if they match the expected output, we print `vcpu->kvm_run->io.size` bytes stored at `vcpu->kvm_run->io.data_offset`.
Both of which are set by the `outb` command.

Finally, the guest writes the number 42 to memory and EAX register, and then
executes the `hlt` instruction.

### Question a.7

At what guest virtual (and physical?) address is the number 42 written?
And how (and where) does the hypervisor read it?

### Answer a.7

The number is written both to address `0x400` and to `rax`.
Once using: `*(long *)0x400 = 42` and once as an argument to the `hlt` inline assemblyl command.
When the hypervisor sess the `KVM_EXIT_HLT` reason, it first checks the `rax` value,
and then reads from the `0x400` address using: `memcpy(&memval, &vm->mem[0x400], sz);`

**(b) Extend with new hypercalls**

Implemented. See code at commit [e10baf28b1c1817d426acf61b00d4da75b38ae54](https://github.com/academy-dt/kvm-hello-world/commit/e10baf28b1c1817d426acf61b00d4da75b38ae54)

**(c) Filesystem para-virtualization**

Implemented. See code at commit [e10baf28b1c1817d426acf61b00d4da75b38ae54](https://github.com/academy-dt/kvm-hello-world/commit/e10baf28b1c1817d426acf61b00d4da75b38ae54)

**(d) Bonus: hypercall in KVM**

Not implemented

**(e) Multiple vCPUs**

Implemented e.1 using actual code, including:
- Virtual memory changes.
- Creating additional VCPUs.
- Fixed virtual memory base address propagation to all functions.
- Fixed RIP/RSP and segment definitions.
- Running everything from dedicated threads.
- Per-CPU members for all hypercalls.
- Easy configuration for more than 2 CPUs

But still, something doesn't work, and we don't know what.
Guest exists with SHUTDOWN the moment we call VMENTER for the second VCPU.

## (2) Containers and namespaces

In this part of the assignment, we've been sent to read and fully understand the basics of Linux namespaces, and practicly implement a simples container runtime that can spawn a command in an isolated environment. <br/>
The first stage was to build, step by step, a fully isolated environment for a given process, as described: <br/>

(1) <ins>Creating user namespace:</ins><br/>
   First thing we will start with the child shell:<br/>
   <img src="/images/01_createUsernsChild.png"> <br/><br/>
   And now we will procceed in the parent shell:<br/>
   <img src="/images/02_createUsernsParent.png"> <br/><br/>
   And now we will got back to the child shell:<br/>
   <img src="/images/03_createUsernsChild.png"> <br/><br/>
(2,3) <ins>Creating uts and ipc namespaces; </ins><br/>
   <img src="/images/04_createUtns&IpcnsChild.png "> <br/> <br/>
(4) <ins>Creating net namespace; </ins><br/>
   <img src="/images/05_createNetnsChild.png"> <br/>
   <img src="/images/06_createNetnsParent.png"> <br/><br/>
   Now, if we type `ip link`, we will see our only 2 interfaces in the namespaces in DOWN mode:<br/>
   <img src="/images/07_ipLink.png"> <br/><br/>
   We will change to UP mode:<br/>
   <img src="/images/08_ipLink.png"> <br/><br/>
   Last step, we will configure the ip address of device `peer0` and `ping` it with 1 ping to check it's connectivity:<br/>
   <img src="/images/09_ipAddAndPing.png"> <br/><br/>
(5,6) <ins>Creating pid and mnt namespaces: </ins><br/>
   <img src="/images/10_createPidsnsAndMntns.png"> <br/><br/>

### Question (a.1)
Describe the process hierarchy produced by the sequence of commands in the "child shell" column. <br/>
### Answer (a.1)
In the "child shell", we made a few commands in order to create the several namespaces we wanted to. <br/>
<ins>(1) USER namespace </ins><br/>
First, in line 3, we're creating a new usrns, with the command `unshare /bin/bash/`, which moves the bash program to a new namespace (unshares the current ones). <br/>
In our case, we use the flag `-U`, which means "User", and ment to `unshare` the user namespace. In the same command, we run `--kill-child`, which means that when the `unshare` will terminate, it will have signame be sent to the forked child process. <br/>
In line 4, we run `echo "my-user-ns" > /proc/$$/comm`, which replaces the comm value, which is the command name associated with the process of the file from "bash" to "my-user-ns". The purpose of changing the `bash`'s COMM to "mu-user-ns" is for the next step at the "parent shell" (line 9), where we will `grep` the process and show that it is still appears authentically in the "parent shell", and without the COMM changing it will be hard to find.<br/>
Then, we run `id` which represent our, UID, GID and Groups:<br/>
```
uid=65534(nobody) gid=65534(nogroup) groups=65534(nogroup)
```
The "65534" in the UID, GID and groups, represents the user `nobody`, which means that they have no mapping inside the namespace. The value for user `nobody` is UNIX standard and defined in the file `/proc/sys/kernel/overflowuid`. When we creare USER namespace, the new user has no mapping and that is the cause. <br/><br/>
<ins>(2,3) UTS and IPC namespaces </ins><br/>. 
In line 20, we create IPC namespace and UTS namespace in the same command, when we run `unshare --ipc --uts --kill-child /bin/bash`. <br/>
Using different flags (`--ipc` or `--uts` or `-U`, etc) only indicates which kind of namespace we would like to create by calling the `unshare` command. Same as at the USER namespace, we also use the `--kill-child` flag. <br/>
After we create the new HOSTNAME namespace (uts), we would like to change its name to "isolated", and we do it by running `hostname isolated`. <br/>
Next, we run `hostname` just to check that the name of the new UTS namespace has been changed to "isolated". <br/><br/>
<ins>(4) NET namespace </ins><br/>
We start as we did in the other namespaces, and create a NET namespace whitin the command `unshare --net --kill-child /bin/bash` (line 26). <br/>
Then, in line 27 we will change again the COMM value with the command `echo "my-net-ns" > /proc/$$/comm`. We can see the use to `grep` again the process in line 29 (at the parent shell). <br/>
After we've created (lines 32-36) the virtual network interface pair on (veth0-peer0), turned it on and assigned an IP to the veth0 interface, and put the peer0 interface in the previously created network namespace and keep the other end in the root network namespace, it is time to configure the pair in the "child shell". <br/>
In line 38, we first check with `ip link` what interfaces do we have in this NET namespace. <br/>
After we see in lines 39-42 both of the interfaces (loopback and peer0) and that they are down, we run 2 commands in lines 43-44 to bring them to UP mode.

     43                                  $ ip link set lo up
     44                                  $ ip link set peer0 up

Now, all we need to do is to assign an IP to `peer0` inteface with the command `ip addr add 10.11.12.14/24 dev peer0`.<br/>
And ofcourse, we need to check that we have connection with pinging the address "10.11.12.13" (line 47). <br/>
We can see that the ping worked :) <br/>

     47                                  $ ping -c 1 10.11.12.13
     48                                  PING 10.11.12.13 (10.11.12.13) 56(84) bytes of data.
     49                                  64 bytes from 10.11.12.13: icmp_seq=1 ttl=64 time=0.066 ms


<ins>(5,6) PID and MNT namespaces </ins><br/>
In line 53, we create PID namespace and MNT namespace in the same command. <br/>
As we know, in Linux there is only 1 process tree, enumerating all of its running processes in the OS from 1 (which is the process `systemd()`) to n processes.
When creating a Process namespaces (PIDNS), we make nested process trees. <br/>
We give the processes (other than systemd) the option to be the root process by moving on the top of a subtree. It means that in that tree, the process will get the PID = 1 (such as the `systemd()` in the OS). The other processes' in that nested namespace will receive their number realtively to the nested root process in that subtree. <br/>
So, in the command in line 53 we used `unshare --pid --mount --fork --kill-child /bin/sh`, which means we moved the current process of `/bin/sh/` to a new set of namespace and actually unshared it from its curret one with the `unshare` syscall.<br/>
When we used the flag `--pid`, we've set that from now on, children will have a distinct set of PID-to-process mappings from their parent. <br/>
In order to make it "happen", we had to use the `--fork` flag, which forked the program as a child process of `unshare`rather than running it directly.<br/>
We also run `--kill-child`, which sends the command to kill the subtree of all processes created under the forked child process, after the `unshare` will be terminated.
In the same command, we run the flag `--mount`, which was supposed to `unshare` the mounted filesystem and create a new Mount namespace.<br/>
In line 54, we run the command `mount -t proc proc /proc` <br/> in order to mount the filesystem with the type (flag `-t`) of proc filesystem which is mounted at `/proc`. <br/>
And finnaly, we run `ps` (line 55) to check which processes are running in our nested subtree. <br/>

### Question (a.2)
How can it be minimized, and what would the hierarchy look like?
### Answer (a.2)
The hierarchy presented in Answer (a.1) can be minimized by merging some of the commands together, for example: <br/>
```#!/bin/bash
        Parent shell                     Child shell
        -------------------------------  -----------------------------  
      1                                  # (1) create (privileged) userns
      2   
      3                                  $ unshare -U --kill-child /bin/bash
      4                                  $ echo "my-user-ns" > /proc/$$/comm
      5                                  $ id
      6                                  uid=65534(nobody) gid=65534(nogroup) groups=65534(nogroup)
      7   
      8   
      9   $ ps -e -o pid,comm | grep my-user-ns
     10   22310,my-user-ns?
     11   
     12   $ sudo bash -c 'echo "0 1000 1000" > /proc/22310/uid_map' && sudo bash -c 'echo "0 1000 1000" > /proc/22310/gid_map'
     15                                  $ id
     16                                  uid=0(root) gid=0(root) groups=0(root),65534(nogroup)
     17                                  
     18                                  # (2,3,4) create utsns, ipcns and netns
     19   
     20                                  $ unshare --ipc --uts --net --kill-child /bin/bash
     21                                  $ hostname isolated
     22                                  $ hostname
     23                                  isolated  
     27                                  $ echo "my-net-ns" > /proc/$$/comm
     28   
     29   $ ps -e -o pid,comm | grep my-user-ns
     30   22331,my-net-ns?
     31   
     32   $ sudo ip link add veth0 type veth peer name peer0
     33   $ sudo ip link set veth0 up
     34   $ sudo ip addr add 10.11.12.13/24 dev veth0
     35   
     36   $ sudo ip link set peer0 netns /proc/22331/ns/net
     37   
     38                                  $ ip link
     39                                  1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN mode DEFAULT group default qlen 1000
     40                                      link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
     41                                  9: peer0@if10: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
     42                                      link/ether 76:8d:bb:61:1b:f5 brd ff:ff:ff:ff:ff:ff link-netnsid 0
     43                                  $ ip link set lo up && ip link set peer0 up
     45                                  $ ip addr add 10.11.12.14/24 dev peer0
     47                                  $ ping -c 1 10.11.12.13
     48                                  PING 10.11.12.13 (10.11.12.13) 56(84) bytes of data.
     49                                  64 bytes from 10.11.12.13: icmp_seq=1 ttl=64 time=0.066 ms
     50   
     52                                  # (5,6) create pidns, mntns
     53                                  $ unshare --pid --mount --fork --kill-child /bin/sh
     54                                  $ mount -t proc proc /proc
     55                                  $ ps
```
### Question (b)
What would happen if you change the order of namespace creation, e.g. run `unshare --ipc` first? <br/>
And what would happen if you defer lines 12-13 until a later time?
### Answer (b)
### Question (c)
What is the purpose of line 4 and lines 9-10 (and similarly, line 27 and lines 29-30)? Why are they needed?<br/>
### Answer (c)
The purpose is to send "signals" to the parent process in order to find which processes are those we are looking for.
`$$` is a variable and represent the PID.
As we explained the commands in Answer (a.1), in order to make that happen, we are changing the COMM value in its `/proc/$$/COMM` file, with the command `echo "my-net-ns" > /proc/$$/comm`, and when we get to the "parent shell" we can simply `grep` "my-user-ns" on the results of the `ps` command. <br/>
### Question (d.1)
Describe how to undo and cleanup the commands above. (Note: there is more than one way; try to find the minimal way). <br/>
Make sure there are no resources left dangling around.
### Answer (d.1)
The minimal and simpliest way is just to run `exit` in both shells. <br/>
The only thing that can be left dangling around is the network configurations in the "parent shell", and those can be undo by just running `ip link delete dev veth0` in the "parent shell". <br/>
Of course, we should undo first the network configuration and then `exit` from all the shells. <br/><br/>
### Question (d.2)
Write a program that would implement the sequence above, whose usage is:

    usage: isolate PROGRAM [ARGS...]

For example, the command:

    isolate ps aux

would execute the command "ps aux" inside an isolated environment.
### Answer (d.2)
Implemented. See code at [isolate.c](/isolate.c).
### Question (e)
Test your program. Does it require root privileges? If so, then why? How can it be changed to not require these privileges?
### Answer (e)
It DOES require root privilieges, since it is going to change network interfaces and to modify other processes.

