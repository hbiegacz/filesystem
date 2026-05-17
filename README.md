# System plików
![Static Badge](https://img.shields.io/badge/c%2B%2B-00000?style=for-the-badge&logo=c%2B%2B&logoSize=auto&labelColor=blue&color=blue) ![Static Badge](https://img.shields.io/badge/GNU-BASH-00000?style=for-the-badge&logo=gnubash&logoSize=auto) ![Static Badge](https://img.shields.io/badge/cmake-00000?style=for-the-badge&logo=cmake&logoSize=auto&labelColor=%23064F8C&color=%23064F8C)



Projekt stworzony w ramach przedmiotu _Systemy Operacyjne_ realizowanego na Politechnice Warszawskiej.

**Cel**: Stworzenie na dysku systemu plików z wielopoziomowym katalogiem oraz aplikacji konsolowej do jego obsługi.

Aplikacja powinna umożliwiać:
- Tworzenie pliku dysku wirtualnego.
- Kopiowanie plików między dyskiem systemowym a wirtualnym.
- Tworzenie i usuwanie katalogów na dysku wirtualnym.
- Wyświetlanie katalogów wraz z rozmiarami plików, katalogów i ilością wolnego miejsca.
- Tworzenie twardych dowiązań do plików i katalogów.
- Usuwanie plików i dowiązań.
- Dodawanie lub usuwanie n bajtów w plikach.
- Wyświetlanie zajętości dysku wirtualnego.

**Wymagania**: Obsługa wielopoziomowej struktury katalogów i intuicyjny interfejs konsolowy.

# Instrukcja obsługi
Pobierz repozytorium 
```bash
  git clone https://github.com/hbiegacz/SOI_FILESYSTEM.git
```
Wejdź do katalogu filesystem i wykonaj następujące komendy:
```bash
  mkdir build
  cd build
  cmake ..
  make
  ./filesystem
```
