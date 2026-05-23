# Semantic Analysis - Milestone 3 - Tugas Besar 1 IF2224 Teori Bahasa Formal dan Otomata 2025/2026

## Deskripsi Program

Program ini merupakan implementasi Milestone 3 dari compiler front-end bahasa pemrograman Arion, dengan fokus utama pada tahap **semantic analysis**. Tahap lexical analysis dan syntax analysis yang telah dibuat pada milestone sebelumnya digunakan sebagai dasar untuk menghasilkan parse tree dari source code Arion. Parse tree tersebut kemudian dikonversi menjadi **Abstract Syntax Tree (AST)**, lalu diproses lebih lanjut menjadi **Decorated Abstract Syntax Tree (Decorated AST/DAST)**.

AST digunakan sebagai representasi program yang lebih ringkas dibandingkan parse tree. Informasi sintaksis yang tidak lagi dibutuhkan untuk analisis semantik, seperti tanda kurung, titik koma, dan keyword penanda blok, tidak lagi disimpan sebagai node utama. Sebaliknya, AST menyimpan struktur yang bermakna secara semantik, seperti assignment, operasi biner, operasi unary, pemanggilan procedure/function, akses array, akses field record, dan deklarasi program.

Pada tahap semantic analysis, AST ditelusuri menggunakan fungsi visitor pada kelas `DecoratedAST`. Setiap node yang dikunjungi akan diproses sesuai jenisnya untuk membangun **symbol table**, melakukan pengecekan tipe, melakukan pengecekan scope, dan menambahkan annotation pada node AST. Annotation tersebut dapat berupa nama tipe, indeks pada `tab`, indeks pada `atab`, indeks pada `btab`, serta lexical level. Hasil akhir dari tahap ini adalah Decorated AST, yaitu AST yang telah dilengkapi informasi semantik.

Symbol table pada program ini dibagi menjadi beberapa struktur. `tab` digunakan untuk menyimpan informasi identifier seperti program, konstanta, tipe, variabel, parameter, procedure, function, dan field record. `atab` digunakan untuk menyimpan informasi tipe array, seperti tipe indeks, batas bawah, batas atas, tipe elemen, ukuran elemen, dan ukuran total array. `btab` digunakan untuk menyimpan informasi block atau scope, seperti program, procedure, function, record, dan block anonim. Selain itu, terdapat type descriptor untuk menyimpan informasi tipe khusus seperti subrange dan enumerated type.

Program akan membaca input source code Arion dalam file `.txt`, lalu menghasilkan output ke folder `test/milestone-3` dengan format:

```text
dast-<nama-file-input>
```

Output tersebut berisi:

- `tab`: tabel identifier, seperti program, constant, type, variable, procedure, function, parameter, dan field.
- `atab`: tabel khusus tipe array.
- `btab`: tabel block/scope, seperti program, procedure, function, record, dan anonymous block.
- `type`: tabel descriptor untuk tipe tambahan seperti subrange dan enumerated.
- Decorated AST: AST yang sudah diberi annotation hasil semantic analysis.

## Alur Program

```text
Source Code
-> Lexer
-> Token
-> Parser
-> Parse Tree
-> ASTBuilder
-> AST
-> DecoratedAST
-> Symbol Table + Decorated AST
```

## Komponen Utama

- `Tokenizer`: melakukan lexical analysis dan menghasilkan token.
- `Parser`: melakukan syntax analysis dengan Recursive Descent dan menghasilkan parse tree.
- `ASTBuilder`: mengubah parse tree menjadi AST yang lebih ringkas.
- `AST`: struktur node AST dan annotation.
- `SymbolTable`: menyimpan informasi identifier, array, block, dan type descriptor.
- `DecoratedAST`: melakukan semantic analysis, mengisi symbol table, mengecek aturan semantik, dan mencetak DAST.

## Semantic Analysis

Semantic analysis mengecek apakah program valid secara makna, bukan hanya valid secara grammar. Beberapa contoh pengecekan yang dilakukan:

- Identifier harus sudah dideklarasikan sebelum digunakan.
- Identifier tidak boleh dideklarasikan ulang pada scope yang sama.
- Assignment harus menggunakan tipe yang compatible.
- Kondisi `if`, `while`, dan `repeat-until` harus bertipe boolean.
- Operasi aritmatika, logika, dan relasional harus menggunakan operand yang sesuai.
- Pemanggilan procedure/function harus sesuai jumlah dan tipe argumen.
- Akses array harus menggunakan tipe index yang sesuai.
- Akses field record harus merujuk field yang memang ada.
- Range/subrange harus memiliki batas bertipe sama dan valid.
- Enumerated type tidak boleh memiliki elemen duplikat.

Jika terjadi syntax error, program berhenti pada tahap parser sehingga AST/DAST tidak dibuat. Jika syntax valid tetapi semantic error, program berhenti pada tahap DecoratedAST dan menampilkan pesan semantic error.

## Requirements

- C++17 atau lebih
- Makefile

## Instalasi

### Windows (MSYS2 + MinGW-w64)

Download dan install MSYS2 dari https://www.msys2.org/

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

Saat program dijalankan, masukkan path file source code Arion yang ingin dianalisis. Contoh:

```bash
make run
Input file path: test/milestone-3/input-1.txt
```

Output Decorated AST dan Symbol Table akan disimpan, misalnya:

```text
test/milestone-3/dast-input-1.txt
```

## Contoh Semantic Error

Contoh program:

```pascal
program SalahSemantik;

var
  a: integer;
  b: boolean;

begin
  a := b
end.
```

Program tersebut valid secara syntax, tetapi tidak valid secara semantic karena `a` bertipe `integer`, sedangkan `b` bertipe `boolean`. Program akan menghasilkan pesan error seperti:

```text
Semantic error: Incompatible assignable type: integer := boolean
```

## Test Case

Test case Milestone 3 berada di:

```text
test/milestone-3
```

## Author

Kelompok ReguExceptional - RGX:

1. Wafiq Hibban Robbany - 13524016 - @wafhr
2. Muhammad Nafis Habibi - 13524018 - @NafisKreatif
3. An-Dafa Anza Avansyah - 13524038 - @An-Dafa
4. Wildan Abdurrahman Ghazali - 13524054 - @wzlyy

## Pembagian Tugas

| Nama | NIM | Pembagian Tugas | Kontribusi (%) |
| -------- | -------- | -------- | -------- |
| Wafiq Hibban Robbany | 13524016 | ASTNode, ASTBuilder | 24 |
| Muhammad Nafis Habibi | 13524018 | Decorated AST | 28 |
| An-Dafa Anza Avansyah | 13524038 | ASTBuilder | 24 |
| Wildan Abdurrahman Ghazali | 13524054 | Symbol Table | 24 |