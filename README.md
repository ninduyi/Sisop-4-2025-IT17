# Praktikum Sistem Operasi Modul 4 - IT17

## Anggota Kelompok

| NRP        | Nama                            |
|:----------:|:-------------------------------:|
| 5027241006 | Nabilah Anindya Paramesti       |
| 5027241092 | Muhammad Khairul Yahya          |
| 5027241002 | Balqis Sani Sabillah            |


## Daftar Isi

- [Soal 1](#soal-1)
- [Soal 2](#soal-2)
- [Soal 3](#soal-3)
- [Soal 4](#soal-4)

# Soal 1
_**Oleh : Nabilah Anindya Paramesti**_

## Deskripsi Soal
Suatu hari, saat di Tethys' Deep, Shorekeeper menemukan sebuah anomali yang baru diketahui. Anomali ini berupa sebuah teks acak yang kelihatannya tidak memiliki arti. Namun, ia mempunyai ide untuk mencari arti dari teks acak tersebut.

## Kesimpulan soal
### Tujuan Program
1. **Mengambil sampel anomali teks**  
   Program harus mengunduh file ZIP yang berisi sampel teks dari sebuah link dan mengekstraknya. Setelah ekstraksi selesai, file ZIP harus dihapus.

2. **Identifikasi Format Teks**  
   Teks yang diambil dalam file ZIP adalah dalam format hexadecimal.

3. **Konversi Hexadecimal ke Gambar**  
   Program harus mengubah string hexadecimal dalam teks menjadi sebuah file gambar.

4. **Penamaan File Gambar**  
   Penamaan file gambar yang dihasilkan dari konversi harus mengikuti format berikut:  
   `[nama file string]image[YYYY-mm-dd]_[HH:MM:SS].png`

   Contoh:  
    `1_image_2025-05-11_18:35:26.png`

5. **Menyimpan Gambar di Folder "image"**  
    Gambar yang dihasilkan dari konversi harus disimpan di dalam folder bernama `image`.

6. **Pencatatan ke dalam Log**  
    Setiap proses konversi harus dicatat dalam file log bernama `conversion.log` dengan format berikut:  
    `[YYYY-mm-dd][HH:MM:SS]: Successfully converted hexadecimal text [nama file string] to [nama file image].`

    Contoh log:  
    `[2025-05-11][18:35:26]: Successfully converted hexadecimal text 1.txt to 1_image_2025-05-11_18:35:26.png.`


7.  Fitur Program
    - Membaca file teks dengan format hexadecimal.
    - Mengubah hexadecimal ke format gambar.
    - Menyimpan gambar di folder yang benar.
    - Menghapus file ZIP setelah proses unzip.
    - Menulis log dengan informasi konversi.


## Jawaban

## Dokumentasi
