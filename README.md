# Syntax Analysis - Milestone 2 - Tugas Besar 1 IF2224 Teori Bahasa Formal dan Otomata 2025/2026 

## Deskripsi Program

Program ini merupakan implementasi tahap lexical analysis dan syntax analysis untuk bahasa pemrograman Arion. Source code Arion dalam file `.txt` pertama-tama diproses oleh lexer untuk diubah menjadi daftar token. Token-token tersebut kemudian diberikan kepada parser untuk diperiksa susunan sintaksnya berdasarkan grammar Arion.

Komponen lexer dibangun menggunakan Deterministic Finite Automata (DFA). Komponen parser dibangun dengan algoritma Recursive Descent, yaitu setiap non-terminal pada grammar direpresentasikan sebagai fungsi parser tersendiri. Parser akan membaca token secara berurutan, melakukan validasi syntax, dan membangun parse tree yang merepresentasikan struktur program.

Program akan membaca input source code Arion dalam file `.txt`, lalu menghasilkan parse tree ke terminal dan menyimpannya ke file output di folder `test/milestone-2` dengan format nama `parsed-<nama-file-input>`.

## Requirements

- C++17 atau lebih
- Makefile

## Instalasi 

### Windows (MSYS2 + MinGW-w64)

Download dan install MSY2 dari https://www.msys2.org/
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

Dengan Makefile, cukup lakukan perintah
```
# Opsional, hapus hasil compile sebelumnya
make clean

# Melakukan compile
make all

# Melakukan compile sekaligus menjalankan program
make run

# Melakukan compile sekaligus menjalankan program dengan output untuk debug
make run-debug
```

Saat program dijalankan, masukkan path file source code Arion yang ingin diparse. Contoh:

```bash
make run
Input file path: test/milestone-2/input-1.txt
```

Output parse tree akan disimpan, misalnya:

```text
test/milestone-2/parsed-input-1.txt
```

Jika terdapat kesalahan syntax, parser akan menampilkan pesan error yang berisi token yang tidak sesuai dan token/struktur yang diharapkan.

## Author

Kelompok ReguExceptional - RGX:

1. Wafiq Hibban Robbany	-	13524016 - @wafhr
2. Muhammad Nafis Habibi - 13524018 - @NafisKreatif
3. An-Dafa Anza Avansyah - 13524038 - @An-Dafa
4. Wildan Abdurrahman Ghazali -	13524054 - @wzlyy

## Pembagian Tugas

| Nama | NIM | Pembagian Tugas | Kontribusi (%) |
| -------- | -------- | -------- | -------- |
| Wafiq Hibban Robbany	|	13524016 | Membuat Diagram DFA | 23 |
| Muhammad Nafis Habibi | 13524018 | Membuat Class DFA | 25 |
| An-Dafa Anza Avansyah | 13524038 | Membuat Diagram DFA | 24 |
| Wildan Abdurrahman Ghazali |	13524054 | Membuat Class Tokenizer (Lexer) | 28 |



