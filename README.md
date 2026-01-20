# System plików
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

# WSKAZÓWKI
- Nie w minixie
- Na starcie tworzy plik binarny (ten wirtualny)
- Można przeczytac jak jest w EXT
- Można ograniczyć rozmiar pliku do kilku nodów? - tak miałem zapisane na zajęciach, ale raczej napewno chodziło o ograniczenie liczby bloków
- Dzielimy plik binarny na 5 częsci
    1. **superblock** (data utworzenia, całkowity rozmiar, wolny rozmiar - tylko potrzebne rzeczy do funkcjonalności)
    2. **tablica node'ów** (hardoced maks rozmiar eltów). Node ma informację o
     - data utworzenia pliku
     - rozmiar pliku
     - pointery na bloki dane
     - bool isDir
     - licznik dowiązań twardych
     - NIE MOŻNA MIEĆ tutaj nazwy pliku
    3. **bitmapa zajętości bloków nodów**
    4. **bitmapa zajętości bloków danych**
    5. **bloki danych** - stały rozmiar