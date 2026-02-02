## Background
  
### Intel-VTx and AMD-V

Hardware-assisted virtualization was introduced to address the performance and isolation
limitations of purely software-based virtual machines. Intel VT-x and AMD-V are CPU
virtualization extensions that allow a hypervisor to execute guest operating systems directly
on the processor while maintaining strict control over privileged operations. These
technologies form the foundation of modern emulators and hypervisors, including those
used by the Android Studio emulator on Windows systems.

A critical contribution of VT-x and AMD-V is the introduction of virtualization-specific
instructions that enable controlled transitions between guest and hypervisor contexts. On
Intel processors, instructions such as VMXON and VMXOFF enable and disable
virtualization mode, while VMLAUNCH and VMRESUME are used to start or resume
guest execution. When a guest executes certain privileged instructions or encounters
configured events, the CPU triggers a VM exit, transferring control back to the hypervisor.

Windows Hypervisior API serves as a user mode wrapper for these instruction which will be used.

## Method

Qemu/android emulator uses Windows hypervisior API for hardware acceleration on CPU. By dropping a proxy WinHvPlatform.dll, we can intercept and gain control over the creation of the virtual machine instance.

At runtime, we can resolve the target function’s location and scan memory for the relevant bytecode, or obtain the VA of kernel functions and translate it to a PA. Execute permission is then removed from the desired memory page, causing a VM exit when execution reaches it. At that point, we can safely tamper with CPU registers or memory before resuming execution.

This type of hooking method has several advantages, the biggest being its stealth, as it is one of the most effective ways to tamper with memory and CPU registers at runtime without leaving obvious traces.

## PoC

This PoC application checks for specific file which are only present in emulators and returns output "Emulator detected" if files exist other wise "No Emulator detected".
PoC application stores result of checks in first parameter of the function which is being tampered to change the output.



https://github.com/user-attachments/assets/544b95fb-b4e4-4e09-88ee-86c5e69f04b2

## Future Prospects
This project was completed under very tight time constraints, and significant work remains to be done. Combining Frida for Java/Kotlin-level hooking with hypervisor-based hooking for native libraries and kernel-level components can provide an exceptionally high degree of control over the target system.
Games and banking applications present the most interesting targets, as they rely heavily on aggressive virtual machine checks and integrity validation mechanisms.

## Contact
Email - krx3x@proton.me

Discord - @ayushkr3
