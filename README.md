# "Advanced Topics in Operating Systems" 
 M.Sc. course at the Reichman University <br/>
 Lecturer and guide - Dr. Oren Laadan <br/>
## Intro

This program is a part of our 3rd assignment. <br/>
Check out [os-hw3/assignment-3.md](https://github.com/GlaiChen/os-hw3/blob/main/assignment-3.md) for full details of the assignment. <br/>
This program is only the invidual part, and the group part will appear at //%%%%// from our repo at the following link: <br/>https://github.com/academy-dt/adv-os-linux/tree/ex3 <br/>

## Invidual Part - Kernel Virtual Machine
In several lectures in the course, we have learned about virtualization from most of its aspects. <br/>
1. In Lecture #9, we've been shown by Dr. Laadan 2 mechanisms (you can find it [here](https://github.com/GlaiChen/os-hw3/blob/main/AOS-2022-L09.pdf), at slides 26 && 27). <br/>
The first mechanism is of KVM as despicted in slide 26: <br/><br/>
<img src="/images/slide_26.jpg"> <br/><br/>
First, it is Type II hypervisor,means it is a hosted hypervisor, which support guest virtual machines by coordinating calls for CPU, memory and other resources through the physical host's operating system. In simple words -type II runs on top of an OS. <br/>
In our case study, the KVM has been written as linux kernel module and for linux and Intel VT-x purpose. <br/>
They actually virtualized only the CPU and MMU, and all of the rest of resources, such as BIOS, devices, etc, have been exported to the User Space and relied on an emulator project called QEMU and used this architecture emulator to make all of the "jobs" they did not want to make inside the Kernel Space. <br/> 
**To be Continued...**<br/>

2. When the OS runs out of memory it resorts to swapping pages to storage. Considering a system with QEMU/KVM hypervisor running several guests:
<br/>
   a. If the guest runs out of memory and starts swapping <br/>
   **To be Continued...**<br/>
   b. When the host runs out of they memory, and swaps out pages that are used by the hypervisor, <br/>
   **To be Continued...**<br/>
3. One difference between plain virtualization and nested virtualization is that the former can leverage EPT/NPT hardware extensions, while the latter cannot do so directly <br/>
   a. <br/>
    **To be Continued...**<br/>
   a. <br/>
    **To be Continued...**<br/>
4. The [Antfarm project](https://research.cs.wisc.edu/wind/Publications/antfarm-usenix06.pdf) relies on shadow page-tables for introspection, to learn about processes inside the guest. It was built before the introduction of EPT/NPT. <br/>
 **To be Continued...**<br/>

