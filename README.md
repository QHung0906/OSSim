# demacOprx - OS Simulation for CO2018

Dự án mô phỏng hệ điều hành thu nhỏ môn Hệ Điều Hành. Hệ thống gồm bộ điều phối tiến trình đa mức MLQ, quản lý bộ nhớ phân trang đa cấp 64 bit và bộ đồng xử lý phần cứng DMA kết hợp Verilator.

---

## Bảng Phân Công Nhiệm Vụ 5 Thành Viên

Dựa theo kết quả bình chọn chính thức của nhóm:

| Thành viên | Vị trí đảm nhiệm | Module chính |
| :--- | :--- | :--- |
| **Hải** | Bộ điều phối tiến trình MLQ và vòng lặp CPU `scheduler-mlq` | `src/queue.c`, `src/sched.c` |
| **Dương** | Bộ nhớ vật lý RAM, SWAP và hoán trang `memphy-swap` | `src/mm-memphy.c` |
| **Khôi** | Quản lý bộ nhớ ảo và phân trang 64 bit `vm-paging64` | `src/mm64.c`, `src/mm-vm.c` |
| **Huy** | Giao diện System Call và cách ly bộ nhớ `syscall-iso` | `src/sys_mem.c`, `src/libmem.c` |
| **Hưng** | Bộ đồng xử lý DMA, đồng bộ hóa và quản trị Git `dma-async` | `src/dma_c_testbench.cpp`, `src/sysbus.c` |

---

##  Kế Hoạch Tuần 1

Mục tiêu: Khởi động dự án, làm quen luồng Git, hoàn thiện các hàm cấu trúc dữ liệu nền tảng và chạy thử nghiệm các module độc lập.

### 1. Hải - Phân hệ Scheduler MLQ
* File làm việc: `src/queue.c`
* Nhiệm vụ cụ thể:
  - Hoàn thiện hàm `enqueue`: Thêm tiến trình mới vào mảng hàng đợi và tăng kích thước hàng đợi.
  - Hoàn thiện hàm `dequeue`: Lấy tiến trình ra khỏi hàng đợi, dồn các phần tử còn lại lên trước và giảm kích thước hàng đợi.
  - Hoàn thiện hàm `purgequeue`: Tìm và xóa một tiến trình cụ thể khỏi hàng đợi khi tiến trình kết thúc.
* File kiểm tra: Đối chiếu khai báo cấu trúc tại `include/queue.h`.

### 2. Dương - Phân hệ Memphy Swap
* File làm việc: `src/mm-memphy.c`
* Nhiệm vụ cụ thể:
  - Hoàn thiện hàm `MEMPHY_get_freefp`: Lấy số khung trang rảnh đầu tiên từ danh sách liên kết gán cho biến trả về, cập nhật lại con trỏ đầu danh sách và giải phóng node vừa lấy.
  - Hoàn thiện hàm `MEMPHY_dump`: Viết vòng lặp in nội dung các byte trong mảng bộ nhớ ra màn hình theo định dạng hex để phục vụ kiểm tra dữ liệu.
  - Kiểm tra các hàm đọc ghi byte `MEMPHY_read` và `MEMPHY_write` đã có sẵn trong file.
* File kiểm tra: Đối chiếu khai báo cấu trúc tại `include/os-mm.h`.

### 3. Khôi - Phân hệ VM Paging 64 bit
* File làm việc: `src/mm64.c`
* Nhiệm vụ cụ thể:
  - Viết hàm `pte_set_fpn` trong `src/mm64.c`: Dùng các chỉ số 5 cấp đã bóc tách để trỏ đúng vào mục bảng trang thực tế, bật cờ Present, tắt cờ Swapped và ghi số khung nhớ FPN.
  - Viết hàm `pte_set_swap` trong `src/mm64.c`: Trỏ vào đúng mục bảng trang thực tế, bật cờ Swapped, tắt cờ Present và ghi mã loại thiết bị Swap cùng độ lệch Swap offset.
  - Viết hàm `pte_get_entry` và `pte_set_entry` trong `src/mm64.c`: Đọc và cập nhật trực tiếp giá trị 64 bit của mục bảng trang tương ứng.
  - Hoàn thiện hàm `get_pd_from_address`: Bóc tách địa chỉ thành 5 cấp thư mục PGD, P4D, PUD, PMD, PT theo đúng bảng đặc tả.
* File kiểm tra: Đối chiếu các macro tại `include/mm64.h`.

### 4. Huy - Phân hệ Syscall và Cách ly Kernel
* File làm việc: `src/sys_mem.c`
* Nhiệm vụ cụ thể:
  - Triển khai logic tìm kiếm tiến trình thực sự: Dùng tham số `pid` duyệt qua danh sách tiến trình của kernel để tìm đúng tiến trình đang gọi, gán con trỏ tìm được cho biến `caller`.
  - Trả về mã lỗi nếu không tìm thấy tiến trình hợp lệ với `pid` tương ứng.
* File kiểm tra: Đối chiếu danh sách mã thao tác bộ nhớ tại `include/syscall.h`.

### 5. Hưng - Phân hệ DMA Co-processor và Quản trị Repo
* File làm việc: `src/dma_c_testbench.cpp`, `Makefile`
* Nhiệm vụ cụ thể:
  - Khởi tạo Git repository, cấu hình `.gitignore`và đẩy toàn bộ mã nguồn lên GitHub.
  - Biên dịch và chạy thử nghiệm độc lập module DMA trên máy ảo Ubuntu bằng lệnh `make dma` và `./dma_testbench`.
  - Nghiên cứu thiết kế cơ chế chạy bất đồng bộ cho hàm thực thi DMA, chuẩn bị hàm gọi lại `dma_done_callback` khi phần cứng hoàn tất sao chép.
* Cách tự kiểm tra: Chạy lệnh `make dma && ./dma_testbench 0x1000 0x2000 512` kiểm tra việc sao chép dữ liệu trên bộ nhớ.
---

##  Kế Hoạch Tuần 2

Mục tiêu Tuần 2: Lập trình lõi thuật toán chính của từng phân hệ, tự kiểm tra độc lập các module có thể chạy riêng và chuẩn bị các đầu nối giao diện để tuần 3 ghép hệ thống.

### 1. Hải - Phân hệ Scheduler MLQ
* File làm việc: `src/sched.c`, `src/cpu.c`, `src/loader.c`
* Nhiệm vụ cụ thể:
  - Trong `src/sched.c`, viết hàm `get_mlq_proc`: Duyệt lần lượt 140 hàng đợi ưu tiên từ 0 đến 139 theo số slot bằng 140 trừ đi mức ưu tiên. Lấy tiến trình ra và trừ đi 1 slot của hàng đợi đó. Khi hàng đợi hết slot hoặc không còn tiến trình thì chuyển sang hàng đợi kế tiếp. Khi toàn bộ 140 hàng đợi đều hết slot thì khôi phục lại slot ban đầu.
  - Trong `src/sched.c`, viết hàm `put_mlq_proc` và `add_mlq_proc`: Đưa tiến trình trở lại đúng hàng đợi theo mức ưu tiên `proc->prio`.
  - Trong `src/loader.c`: Xử lý ghi đè mức ưu tiên live trong file cấu hình lên mức ưu tiên mặc định của file chương trình.
* Cách tự kiểm tra: Chạy lệnh `make os` và chạy thử kịch bản điều phối đơn CPU bằng lệnh `./os input/os_1_singleCPU_mlq`.

### 2. Dương - Phân hệ Memphy Swap
* File làm việc: `src/mm-memphy.c`
* Nhiệm vụ cụ thể:
  - Viết hàm `find_victim_page`: Triển khai giải thuật thay trang FIFO. Duyệt danh sách các trang đã nạp trong biến `fifo_pgn` của tiến trình, chọn ra trang được đưa vào RAM sớm nhất để làm trang nạn nhân đưa ra SWAP khi RAM hết chỗ.
  - Viết hàm `__swap_cp_page`: Sao chép toàn bộ dữ liệu 256 byte của một trang giữa hai thiết bị bộ nhớ vật lý. Sao chép từ RAM sang SWAP hoặc ngược lại từ SWAP vào RAM.
* File kiểm tra: Đối chiếu khai báo cấu trúc tại `include/os-mm.h` và `include/mm.h`.

### 3. Khôi - Phân hệ VM Paging 64 bit
* File làm việc: `src/mm-vm.c`, `src/mm64.c`
* Nhiệm vụ cụ thể:
  - Trong `src/mm-vm.c`, viết hàm `inc_vma_limit`: Tăng mốc giới hạn sbrk của vùng nhớ hiện tại thêm một kích thước quy định, gọi hàm `validate_overlap_vm_area` để chắc chắn vùng nhớ mới không đè lên vùng nhớ liền kề.
  - Trong `src/mm-vm.c`, hoàn thiện hàm `get_vm_area_node_at_brk`: Cấp phát node `newrg` mới bắt đầu từ mốc sbrk của vùng nhớ hiện tại.
  - Trong `src/mm64.c`, viết hàm `alloc_pages_range`: Tính toán số lượng khung nhớ cần xin cho dải địa chỉ mới, gọi hàm `MEMPHY_get_freefp` của Dương để xin khung nhớ RAM. Nếu hết khung nhớ thì kích hoạt hàm `find_victim_page` của Dương để hoán trang sang SWAP.
  - Trong `src/mm64.c`, viết hàm `vmap_page_range`: Điền các khung nhớ vật lý vừa xin được vào các mục tương ứng trong bảng phân trang 5 cấp của tiến trình.
* File kiểm tra: Đối chiếu các hàm trong `include/mm64.h` và `include/os-mm.h`.

### 4. Huy - Phân hệ Syscall và Cách ly Kernel
* File làm việc: `src/sys_mem.c`, `src/libmem.c`
* Nhiệm vụ cụ thể:
  - Trong `src/sys_mem.c`, hoàn thiện hàm `__sys_memmap`: Dùng tiến trình tìm được qua pid ở tuần 1 để xử lý các mã thao tác bộ nhớ. Mã `SYSMEM_INC_OP` gọi `inc_vma_limit` của Khôi. Mã `SYSMEM_SWP_OP` gọi hoán trang sang SWAP của Dương. Mã `SYSMEM_IO_READ` và `SYSMEM_IO_WRITE` gọi đọc ghi RAM của Dương.
  - Trong `src/libmem.c`, viết hàm `__alloc`: Tìm vùng nhớ rảnh trong danh sách freerg của tiến trình. Nếu không còn chỗ trống, tự động bắn lệnh System Call `SYSMEM_INC_OP` để xin kernel mở rộng vùng nhớ.
  - Trong `src/libmem.c`, viết các hàm `__free`, `__read`, `__write`.
* Cách tự kiểm tra: Chạy kịch bản kiểm tra gọi hệ thống `./os input/os_syscall` để kiểm tra kernel có nhận đúng các mã thao tác hay không.

### 5. Hưng - Phân hệ DMA Co-processor và Bus Addressing
* File làm việc: `src/dma_c_testbench.cpp`, `src/cpu.c`, `include/dma.h`
* Nhiệm vụ cụ thể:
  - Trong `src/dma_c_testbench.cpp`, sửa hàm `dpi_dma_copy`: trỏ trực tiếp địa chỉ nguồn và đích vào vùng nhớ RAM thật của hệ điều hành `caller->krnl->mram->storage`.
  - Trong `src/cpu.c`: Bổ sung xử lý lệnh `DMA_MEMCPY` khi CPU đọc lệnh từ file tiến trình, chuyển các tham số nguồn, đích, độ dài sang hàm sao chép DMA của Hưng.
  - Trong `include/dma.h`: Khai báo và khởi tạo các khóa Mutex dùng chung gồm `queue_lock`, `mem_lock`, `dma_lock`.
* Cách tự kiểm tra: Chạy lệnh `make os && ./os input/os_2_singleCPU_mlq_paging` để kiểm tra CPU đọc lệnh `dma_memcpy` từ tiến trình `m2s` và kích hoạt sao chép DMA trên RAM.

---

## Hướng Dẫn Biên Dịch Trên Ubuntu

```bash
make os    # Build full OS simulation. The output file ./os
```
Run os simulation
```bash
# OS simulation
./os [configuration_file]
```

## Build only DMA module - The output file dma_testbench

```bash
make dma 
```
Run DMA module:
```bash
./dma_testbench
./dma_testbench 0x1000 0x2000 1024
```
## demacOprx environment
Ubuntu Virtual Machine image for Intel-x86 and AARCH64 (required HCMUT.SSO system)
[Course CO2018 Documentation](https://docs.google.com/document/d/1vCxl0sBotFjONqRJBAlnkTVYtlL1XZQrTyVl_dO0zxg/edit?usp=sharing "Course CO2018 Documentation - HCMC University of Technology")

## License

See [LICENSE](LICENSE) — educational use only for CO2018.