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
(1.1) The first mechanism as despicted in slide 26: <br/>
<img src="/images/slide_26.jpg"> <br/><br/>
First, it is Type II hypervisor,means it is a hosted hypervisor, which support guest virtual machines by coordinating calls for CPU, memory and other resources through the host's resources. In simple words -type II runs on top of an OS. <br/>
In our case study, the KVM has been written as linux kernel module and for linux and Intel VT-x purpose and relies on QEMU. <br/>
They actually virtualized only the CPU and/or MMU, and all of the rest of resources, such as BIOS, devices, etc, have been exported to the User Space and relied on an emulator project called QEMU and used this architecture emulator to make all of the "jobs" they did not want to make inside the Kernel Space. <br/> 
In fact, the architecture of that mechanism is based on 3 layers - User mode, Kernel mode and Non root, which is the guest. Both User and Kernel modes are rooted.
First thing first, the user mode process (QEMU) start with ending a syscall to the kernel module, which initate the guest enviorment (allocates memory, loads the BIOS, creates a device, etc),  and then eneters the guest context. <br/>
Once in the guest context, the BIOS start to run. As long as there's no traps, everything continues within the guest context. Once a trap occurs, the kernel module receives it. If it is something that the kernel "knows" how to handle by itself - it simply does that, and returns control back to the guest. Otherwise, the kernel module needs the QEMU's help. In that case, he returns the syscall to the user-root process with the return value holding all the information relevant for the qemu's work. The QEMU then does whatever he needs to do, and stores all the relevant data. It then triggers again the syscall, after completing everything it needed to do, and sending all relevant data. The Kernel module takes the QEMU's data (alongside with possibly things that it also needed to do) and gives back control to the guest. Basically, in this point the Kernel module and the QEMU did everything they were required by the non-root's trap (BIOS trap). This procedure carries on throughout all the guest execution time. <br/><br/>
(1.2) The second mmechanism as despicted in slide 27: <br/>
<img src="/images/slide_27.jpg"> <br/><br/>
Once #vmexit occurs - for any reason that triggers it (the different exit conditions: dividing by zero, page faults, etc...), we move from the guest context to the hypervisor one.  <br/>
 Once the hypervisor receives control, he applies the handler that corresponds to the relevant exit condition. Simple cases are done by the KVM without much hassle. Other cases require the KVM to fetch the command that triggered the trap, decode it, determine if the user has the right privileges, and perform the required steps to execute that instruction (read mem, write memory, update registers). In all of these steps, a fault may occur (GPF/PF while fetching, #UD while decoding, etc...) in which case the emulation will not occur successfully and the hypervisor will reflect that to the guest. Finally, it updates the %rip register (basically skipping the aforementioned instruction), and returns control back to the guest. <br/>
Execution in the guest context continues, with everything taken care of by the hypervisor. If the instruction was authorized - it was executed by the hypervisor. If not, the instruction was not executed and the guest was notified. This is the 'essence' of the virtualization method, allowing for improved security etc...<br/>
2. When the OS runs out of memory it resorts to swapping pages to storage. Considering a system with QEMU/KVM hypervisor running several guests: <br/><br/>
   (a) If the guest runs out of memory and starts swapping, it will try swaping memory in its own filessystem, since he doesn't know it runs on a virtual filessystem and a virtual memory, it treats it like it is a physical memory and has its own paging. <br/>
   So once the guest ran out of memory as we mention, and the kernel of the guest will handle the page table entry (PTE), and swapps it inside the guest, and since he knows only the guests' PTE, it won't have any affect on the host at all. <br/> However, the guest will keep getting page faults as long it won't be solved by the host with an active action. <br/>
  
   (b) In the second hand, when the host is the one who runs out of they memory, and swaps out pages that are used by the hypervisor it's a different situation. If it ran out of memory at the hosts, it will receive page faults at the guest-userspace because the pages doesn't exists.<br/>
   In that situation, it will be recognised as "missing" at the PTE, and will move to the hosts' KVM in order to handle the pages and reload them at the IPT. After it's done, it should solve the page-fault situation and back to the guest-userspace by the `vmentry`. <br/>
   In all of that process, the guest won't understand that something happend, since the host handled the situation. <br/>
   
3. One difference between plain virtualization and nested virtualization is that the former can leverage EPT/NPT hardware extensions, while the latter cannot do so directly <br/>
   (a). <br/>
    **To be Continued...**<br/>
   (b). <br/>
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

(1) Creating user namespace:

First thing we will start with the child shell:

<img src="/images/01_createUsernsChild.png">

And now we will procceed in the parent shell:

<img src="/images/02_createUsernsParent.png">

And now we will got back to the child shell:

<img src="/images/03_createUsernsChild.png">

(2,3) Creating uts and ipc namespaces:

<img src="/images/04_createUtns&IpcnsChild.png">

(4) Creating net namespace:

<img src="/images/05_createNetnsChild.png">

<img src="/images/06_createNetnsParent.png">

Now, if we type `ip link`, we will see our only 2 interfaces in the namespaces in DOWN mode:

<img src="/images/07_ipLink.png">

We will change to UP mode:

<img src="/images/08_ipLink.png">

Last step, we will configure the ip address of device `peer0` and `ping` it with 1 ping to check it's connectivity:

<img src="/images/09_ipAddAndPing.png">

(5,6) Creating pid and mnt namespaces:

<img src="/images/10_createPidsnsAndMntns.png">

### Question (a.1)
Describe the process hierarchy produced by the sequence of commands in the "child shell" column.

### Answer (a.1)
When running `unshare`, the shell creates a new child application by `fork()`-ing and then `exec()`-ing the `unshare` app.
When using the `--kill-child` flag, according to the `man` pages, it infers the `--fork` flag:
> "This option implies --fork""

So unshare doesn't execute the command we give it, but rather create another child by `fork()`-ing and then runs it.
Hence, the process hierarchy is going to be:
child shell > `unshare -U` > `my-user-ns` > `unshare --ipc --uts` > `/bin/bash` > `unshare --net` > `my-net-ns` > `unshare --pid --mount` > `/bin/sh`

Another way to "verify" that is to look at `pstree -p` that prints out all the processes on the machine in a tree form.
However, in this scenarion, the chain is too long for the tool, so we can verify using `ps -ef` and jump from child to parent.

### Question (a.2)
How can it be minimized, and what would the hierarchy look like?

### Answer (a.2)
The only namespace that you can change for yourself effectively is the PID one.
When we call the syscall `unshare(CLONE_NEWPID)`, it only affects the PID namespace for our children:
> Unshare the PID namespace, so that the calling process has a new PID namespace for its children

So, theoretically, we could run all the unshares in a single command with `--fork-child`, creating a single `unshare` process, with a single `/bin/bash` child:
`unshare --pid --ipc --uts --net --user --mount --fork-child /bin/bash`

The chain would be: child shell > `unshare` > `bash`.

The problem with this method, is that it requires using `sudo`, because our child shell is not allowed to create new namespaces.
To work around that, we could first create a new shell in a new user namespace, (without the `--fork-child` to avoid creating an extra process),
then give it permissions from the parent shell, like we've done before, and finally, run an additional `unshare` command with all the other flags.

### Question (b)
What would happen if you change the order of namespace creation, e.g. run `unshare --ipc` first? <br/>
And what would happen if you defer lines 12-13 until a later time?

### Answer (b)
If we change the order of namespace creation, and run any other flag than `-U` (USER namespace),
means we would like to create any of the other 6 kinds of namespaces (IPC, NET, UTS, MNT, PID or CGROUP).

We would receive an error of `unshare: unshare failed: Operation not permitted`, because our child shell is running as an unprivileged user. We could work around that by calling `sudo unshare...`.

But, if we create USER namespace first, and run the 2 code lines which appears in lines 12-13 from the parent shell,
the child shell will obtain "root" privileges inside the new user namespace, allowing it to run additional `unshare` commands:
```
$ sudo bash -c 'echo "0 1000 1000" > /proc/<pid>/uid_map'
$ sudo bash -c 'echo "0 1000 1000" > /proc/<pid>/gid_map'
 ```

When a user namespace is being created, it starts without a mapping of user IDs (UID) and group IDs (GID) to the parent user namespace.

The mapping details of both UID and GID are located at `/proc/<pid num>/uid_map` and `/proc/<pid num>/gid_map` respectively.

When we `cat` the file, what we see is the mapping of UIDs (or GIDs) from the usernamespace of the process pid to the user namespace of the process that opened the file.
The first two numbers specify the starting user ID in each of the two user namespaces.
The third number specifies the length of the mapped range.

In lines 12-13, we are writing to those mapping files with `echo` the range: `0 1000 1000`.
Meaning user `1000` (The `vagrant` user from the original namespace) will be mapped to UID `0` inside the new user namespace, aka `root`.
Thus, the new child shell can now work it's magic.

### Question (c)
What is the purpose of line 4 and lines 9-10 (and similarly, line 27 and lines 29-30)? Why are they needed?

### Answer (c)
The purpose is to help us easily find processes running in nested namespaces from the parent shell.
`$$` is a variable and represent the PID for the current shell.
We are changing the comm value by writing to the `/proc/$$/comm` file, with the command `echo "<name>" > /proc/$$/comm`.

When we get to the "parent shell" we can simply `grep -e -o pid,comm <name>` on the results of the `ps` command.

Is it actually necessary? NO. Why? Because up until you `unshare` the pid namespace, both shells are sharing the pid universe.
If we run `$$` in the child shell at any point before we do that, we'll see the actual pid and are simply able to use it.

### Question (d.1)
Describe how to undo and cleanup the commands above. (Note: there is more than one way; try to find the minimal way).
Make sure there are no resources left dangling around.

### Answer (d.1)
The minimal and simpliest way is just to `exit` in both shells.
Since we are using the `--kill-child` flag, when the topmost `unshare` dies, it sends a signal to all it's children, effectively killing them.

The only thing that can be left dangling around is the network configurations in the "parent shell".
Those can be undo by just running `ip link delete dev veth0` in the "parent shell".

Of course, we should undo first the network configuration and then `exit` from all the shells.

### Question (d.2)
Write a program that would implement the sequence above, whose usage is:

    usage: isolate PROGRAM [ARGS...]

For example, the command:

    isolate ps aux

would execute the command "ps aux" inside an isolated environment.

### Answer (d.2)

### Question (e)
Test your program. Does it require root privileges? If so, then why? How can it be changed to not require these privileges?

### Answer (e)
Yes it does, for the same reasons we've discussed in (b).
It cannot create namespaces without specific permissions to do so.

There are some ways around that:
1. Run that as root, like stated above
1. Use the setuid bit, change the ownership of the binary file to `root` with the `setuid` bit set, this way, the binary will always start with enough capabilities to create the isolated sandbox.
1. Use linux capabilities instead of `root`, giving it more granular permissions by `CAP_SYS_ADMIN` or `CAP_SYS_SETUID`.
