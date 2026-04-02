# Lexical Analysis - Milestone 1 - Tugas Besar 1 IF2224 Teori Bahasa Formal dan Otomata 2025/2026 

## Deskripsi Program

Analisis leksikal (lexical analysis) adalah tahap pertama dalam proses kompilasi atau interpretasi bahasa pemrograman. Pada tahap ini, compiler membaca kode sumber mentah yang pada dasarnya hanyalah rangkaian karakter dan mengubahnya menjadi rangkaian satuan makna yang disebut token. Token merupakan unit terkecil yang memiliki arti dalam sebuah program, misalnya kata kunci (keyword), nama variabel (identifier), operator, angka, atau tanda baca.

Proses ini dilakukan oleh komponen yang disebut lexer. Agar proses pembacaan tersebut dapat dijalankan secara otomatis oleh komputer, lexer harus diimplementasikan ke dalam bentuk Finite Automata, khususnya Deterministic Finite Automata (DFA).

Program akan membaca input file (.txt) lalu meng-output-kannya ke dalam file (.txt) lain berisi token-token yang telah dibaca.

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



