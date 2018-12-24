Tema #2 SPG
Ciupitu Andrei Valentin 
341C4

============================ Detalii de implementare ==========================

Am realizat implementarea in 2 moduri, folosind shadere pe GPU, folosind
intervale de culori si o implementare pe CPU ce foloseste metoda segmentarii 
prin extindere, implementand algoritmul din curs.

In modul GPU, se pot ajusta parametrii filtrului (grosimea liniilor, nivele de
culoare, precizie borduri).

=================================== Controls ==================================

SPACE -> mode
ENTER -> file browser
NUM_MINUS/NUM_PLUS -> Binarization threshold
N/M -> Color levels
O/P -> Dilation radius