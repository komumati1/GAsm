
| 64-bit  | 32-bit | 16-bit | 8-bit | Purpose                  | When calling a function | When writing a function |
| ------- | ------ | ------ | ----- | ------------------------ | ----------------------- | ----------------------- |
| **rax** | eax    | ax     | ah,al | Return value             | Might be changed        | Use freely              |
| **rbx** | ebx    | bx     | bh,bl | Callee-saved             | Will not be changed     | Save before using!      |
| **rcx** | ecx    | cx     | ch,cl | **1st integer argument** | Might be changed        | Use freely              |
| **rdx** | edx    | dx     | dh,dl | **2nd integer argument** | Might be changed        | Use freely              |
| **r8**  | r8d    | r8w    | r8b   | **3rd integer argument** | Might be changed        | Use freely              |
| **r9**  | r9d    | r9w    | r9b   | **4th integer argument** | Might be changed        | Use freely              |
| **r10** | r10d   | r10w   | r10b  | Used for calls (scratch) | Might be changed        | Use freely              |
| **r11** | r11d   | r11w   | r11b  | Used for calls (scratch) | Might be changed        | Use freely              |
| **r12** | r12d   | r12w   | r12b  | Callee-saved             | Will not be changed     | Save before using!      |
| **r13** | r13d   | r13w   | r13b  | Callee-saved             | Will not be changed     | Save before using!      |
| **r14** | r14d   | r14w   | r14b  | Callee-saved             | Will not be changed     | Save before using!      |
| **r15** | r15d   | r15w   | r15b  | Callee-saved             | Will not be changed     | Save before using!      |
| **rbp** | ebp    | bp     | bpl   | Frame pointer (optional) | Might be changed        | Save before modifying   |
| **rsp** | esp    | sp     | spl   | Stack pointer            | Be very careful!        | Be very careful!        |
