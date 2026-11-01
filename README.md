# demacOprx - OS Simulation for CO2018

Dự án mô phỏng hệ điều hành thu nhỏ môn Hệ Điều Hành CO2018 tại Trường Đại học Bách Khoa ĐHQG TP.HCM. Hệ thống bao gồm bộ điều phối tiến trình đa mức MLQ, quản lý bộ nhớ phân trang đa cấp 64 bit và bộ đồng xử lý phần cứng DMA kết hợp Verilator.

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
  - Nghiên cứu cấu trúc phân trang 5 cấp 64 bit gồm PGD, P4D, PUD, PMD, PT trong file `include/mm64.h`.
  - Kiểm tra hàm `get_pd_from_address`: Đối chiếu các công thức dịch bit và mặt nạ bitmask với Bảng 2 Trang 16 trong file đặc tả môn học.
  - Kiểm tra hàm `init_pte`: Nắm rõ cách thiết lập các bit cờ Present, Swapped, Dirty và giá trị khung trang để chuẩn bị cho việc duyệt bảng trang ở tuần kế tiếp.
* File kiểm tra: Đối chiếu các macro tại `include/mm64.h`.

### 4. Huy - Phân hệ Syscall và Cách ly Kernel
* File làm việc: `src/sys_mem.c`
* Nhiệm vụ cụ thể:
  - Xóa bỏ đoạn mã cấp phát tạm thời `malloc` con trỏ giả lập tại dòng 33.
  - Triển khai logic tìm kiếm tiến trình thực sự: Dùng tham số `pid` duyệt qua danh sách tiến trình của kernel để tìm đúng tiến trình đang gọi, gán con trỏ tìm được cho biến `caller`.
  - Trả về mã lỗi nếu không tìm thấy tiến trình hợp lệ với `pid` tương ứng.
* File kiểm tra: Đối chiếu danh sách mã thao tác bộ nhớ tại `include/syscall.h`.

### 5. Hưng - Phân hệ DMA Co-processor và Quản trị Repo
* File làm việc: `src/dma_c_testbench.cpp`, `Makefile`, `.gitignore`
* Nhiệm vụ cụ thể:
  - Khởi tạo Git repository, cấu hình `.gitignore`và đẩy toàn bộ mã nguồn lên GitHub.
  - Biên dịch và chạy thử nghiệm độc lập module DMA trên máy ảo Ubuntu bằng lệnh `make dma` và `./dma_testbench`.
  - Nghiên cứu thiết kế cơ chế chạy bất đồng bộ cho hàm thực thi DMA, chuẩn bị hàm gọi lại `dma_done_callback` khi phần cứng hoàn tất sao chép.

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