# Intermediate Code & Interpreter - Milestone 4 - Tugas Besar IF2224 TBFO 2025/2026

## Deskripsi Program

Program ini merupakan implementasi Milestone 4 dari compiler/interpreter bahasa pemrograman Arion. Pada milestone ini, hasil analisis semantik dari Milestone 3 dilanjutkan menjadi **intermediate code** berbasis **stack machine**, lalu dieksekusi oleh interpreter.

Tahapan lexical analysis, syntax analysis, AST construction, dan semantic analysis tetap digunakan sebagai fondasi. Source code Arion dibaca sebagai teks, diubah menjadi token oleh `Tokenizer`, diparse menjadi parse tree oleh `Parser`, dikonversi menjadi AST oleh `ASTBuilder`, lalu didekorasi oleh `DecoratedAST` dengan informasi symbol table dan tipe. Setelah program valid secara semantik, `IntermediateCodeGenerator` membangkitkan instruksi stack machine, kemudian `Interpreter` menjalankan instruksi tersebut dan menghasilkan output program.

Jalur utama Milestone 4 adalah:

```text
Source Code
-> Tokenizer
-> Parser
-> ASTBuilder
-> AST
-> DecoratedAST + Symbol Table
-> IntermediateCodeGenerator
-> Intermediate Code Stack Machine
-> Interpreter
-> Program Output
```

File `ThreeAddressCode.*` tetap ada sebagai hasil eksplorasi/eksperimen, tetapi **bukan jalur utama eksekusi**. Output utama yang digunakan adalah instruksi stack machine.

## Komponen Utama

- `Tokenizer`: melakukan lexical analysis dan menghasilkan token.
- `Parser`: melakukan syntax analysis dengan recursive descent dan menghasilkan parse tree.
- `ASTBuilder`: mengubah parse tree menjadi AST yang lebih ringkas.
- `DecoratedAST`: melakukan semantic analysis, membangun symbol table, dan memberi annotation pada AST.
- `SymbolTable`: menyimpan informasi identifier, scope/block, array, record, enum, subrange, procedure, function, parameter, dan alamat runtime.
- `IntermediateCode`: merepresentasikan instruksi stack machine.
- `IntermediateCodeGenerator`: mengubah Decorated AST menjadi instruksi stack machine.
- `Interpreter`: menjalankan intermediate code dan menghasilkan output runtime.

## Intermediate Code

Intermediate code direpresentasikan sebagai instruksi stack machine. Instruksi dasar yang digunakan:

```text
LIT  : push literal ke stack
LOD  : load nilai dari address memory
STO  : store nilai ke address memory
CAL  : memanggil procedure/function
INT  : mengalokasikan frame/memory runtime
JMP  : unconditional jump
JPC  : conditional jump
OPR  : menjalankan operasi aritmetika, relasional, boolean, atau output
RET  : kembali dari program/procedure/function
```

Selain instruksi dasar, implementasi ini menambahkan beberapa instruksi untuk mendukung array access dan record field access:

```text
LDA  : push address variabel/field ke stack
IXA  : menghitung address elemen array berdasarkan index
LDI  : load indirect dari address yang ada di stack
STI  : store indirect ke address yang ada di stack
```

Instruksi tambahan tersebut digunakan agar static array access dan dynamic array access dapat dieksekusi oleh interpreter. Konvensi ini perlu dicatat sebagai perluasan implementasi di luar spesifikasi dasar.

## Operation Code

Operasi `OPR` yang digunakan:

```text
NEG   : unary minus
ADD   : penjumlahan
SUB   : pengurangan
MUL   : perkalian
RDIV  : real division untuk operator /
IDIV  : integer division untuk operator div
MOD   : modulo
EQL   : sama dengan
NEQ   : tidak sama dengan
LSS   : kurang dari
GEQ   : lebih dari atau sama dengan
GTR   : lebih dari
LEQ   : kurang dari atau sama dengan
WRT   : write tanpa newline
WRTLN : write dengan newline
I2R   : konversi integer ke real
```

Operator `/` selalu dibangkitkan sebagai `RDIV`, sehingga hasilnya bertipe real. Operator `div` dibangkitkan sebagai `IDIV`, sehingga hasilnya bertipe integer. Operasi `I2R` digunakan saat nilai integer perlu disimpan atau dikirim ke konteks bertipe real, misalnya assignment ke variabel real atau parameter function/procedure bertipe real.

## Fitur Runtime yang Didukung

Interpreter saat ini mendukung eksekusi fitur-fitur berikut:

- Deklarasi dan penggunaan variabel.
- Literal integer, real, boolean, char, dan string.
- Assignment ke variabel, elemen array, dan field record.
- Operasi aritmetika, relasional, dan boolean.
- Percabangan `if-then-else`.
- Percabangan `case-of`.
- Perulangan `while-do`, `repeat-until`, dan `for-to/downto-do`.
- `write` dan `writeln`.
- Procedure call.
- Function call dengan return value melalui assignment ke nama function.
- Static dan dynamic array access.
- Record field access.

Untuk function, Arion mengikuti gaya Pascal. Nilai return diberikan dengan assignment ke identifier function, contohnya:

```pascal
function Square(x: integer): integer;
begin
  Square := x * x
end;
```

## Error Handling Runtime

Interpreter dibuat agar tidak crash saat terjadi runtime error. Beberapa error yang ditangani:

- Instruksi tidak valid.
- Jump target tidak valid.
- Stack underflow.
- Address memory tidak valid.
- Division by zero.
- Operasi tidak didukung.
- Tipe operand tidak sesuai untuk operasi tertentu.
- Array index out of bounds.

Jika error terjadi, program menampilkan pesan error yang menjelaskan penyebab dan lokasi instruksi yang bermasalah.

## Bonus yang Diimplementasikan

Berdasarkan spesifikasi bonus Milestone 4 mengenai vulnerabilities pada interpreter, bagian bonus yang sudah diterapkan penuh adalah:

### Stack Underflow

Interpreter memvalidasi setiap operasi pop melalui helper `popValue`. Jika instruksi mencoba mengambil nilai dari stack kosong, interpreter menghentikan eksekusi dan menghasilkan runtime error, bukan crash.

Contoh pesan error:

```text
Runtime error at instruction <ip>: stack underflow
```

Pengecekan ini digunakan oleh operasi seperti `STO`, `JPC`, `CAL`, `IXA`, `LDI`, `STI`, `WRT`, `WRTLN`, dan operasi `OPR` yang membutuhkan operand dari stack.

### Out-of-Bounds Variable dan Array Access

Interpreter memvalidasi address sebelum melakukan load/store ke memory frame. Untuk akses array, instruksi `IXA` mengecek apakah index berada dalam batas `low..high` yang disimpan pada descriptor array.

Jika index array berada di luar batas, interpreter menghasilkan runtime error seperti:

```text
Runtime error at instruction <ip>: array index out of bounds
```

Validasi ini mencegah akses memory di luar area variabel, field record, atau elemen array yang sah.

### Invalid Jump Targets

Interpreter memvalidasi target instruksi kontrol alur sebelum melakukan lompatan. Instruksi `JMP`, `JPC`, dan `CAL` memanggil validasi target agar instruction pointer tidak melompat ke baris yang tidak ada.

Jika target tidak valid, interpreter menghasilkan runtime error seperti:

```text
Runtime error at instruction <ip>: invalid jump target
```

Dengan validasi ini, kesalahan pada intermediate code tidak menyebabkan interpreter membaca instruksi dari posisi yang tidak sah.

## Output Program

Program membaca path file input Arion, lalu menghasilkan file output ke folder:

```text
test/milestone-4
```

Untuk input:

```text
test/milestone-4/input-1.txt
```

Program menghasilkan:

```text
test/milestone-4/ic-input-1.txt
test/milestone-4/output-input-1.txt
```

Keterangan:

- `ic-<nama-file-input>` berisi intermediate code stack machine.
- `output-<nama-file-input>` berisi output hasil eksekusi interpreter.

## Requirements

- C++17 atau lebih baru
- Makefile
- Compiler C++ seperti MinGW-w64, GCC, atau Clang

## Instalasi

### Windows (MSYS2 + MinGW-w64)

Download dan install MSYS2 dari:

```text
https://www.msys2.org/
```

Buka terminal MSYS2 MinGW 64-bit dan jalankan:

```bash
pacman -S mingw-w64-x86_64-toolchain
```

### Linux (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install build-essential g++ make
```

## Menjalankan Program

Dengan Makefile, gunakan perintah berikut:

```bash
# Opsional, hapus hasil compile sebelumnya
make clean

# Melakukan compile
make all

# Melakukan compile sekaligus menjalankan program
make run

# Melakukan compile sekaligus menjalankan program dengan output debug lexer
make run-debug
```

Saat program dijalankan, masukkan path file source code Arion yang ingin dieksekusi. Contoh:

```text
Input file path: test/milestone-4/input-27.txt
```

Contoh output:

```text
Program output:
sum = 15
5 / 2 = 2.5
5 div 2 = 2
5 mod 2 = 1
logic = true
case branch = 2
Intermediate code outputted to test/milestone-4/ic-input-27.txt
Interpreter outputted to test/milestone-4/output-input-27.txt
```

## Test Case

Test case Milestone 4 berada di:

```text
test/milestone-4
```

Beberapa test case utama untuk laporan:

- `input-27.txt`: arithmetic, real division, integer division, modulo, boolean logic, loop, dan `case`.
- `input-28.txt`: procedure, function, nested function call, dan parameter real.
- `input-29.txt`: static/dynamic array access, multi-dimensional array, char index, dan boolean index.
- `input-30.txt`: record field access, assignment field, `if`, dan `case`.
- `input-31.txt`: integrasi function, real arithmetic, repeat-until, dan pendekatan akar kuadrat.

## Author

Kelompok ReguExceptional - RGX:

1. Wafiq Hibban Robbany - 13524016 - @wafhr
2. Muhammad Nafis Habibi - 13524018 - @NafisKreatif
3. An-Dafa Anza Avansyah - 13524038 - @An-Dafa
4. Wildan Abdurrahman Ghazali - 13524054 - @wzlyy

## Pembagian Tugas

| Nama | NIM | Pembagian Tugas | Kontribusi (%) |
| -------- | -------- | -------- | -------- |
| Wafiq Hibban Robbany | 13524016 | IntermediateCode, IntermediateCodeGenerator | 26 |
| Muhammad Nafis Habibi | 13524018 | IntermediateCode, Testing | 22 |
| An-Dafa Anza Avansyah | 13524038 | Interpreter, Testing | 26 |
| Wildan Abdurrahman Ghazali | 13524054 |  IntermediateCode, IntermediateCodeGenerator | 26 |