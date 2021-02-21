# Święty Mikołaj

Konrad Szychowiak 144564

## Opis zadania

* Święty Mikołaj śpi w swoim domu na biegunie.
* Elfy Mikołaja pracują w fabryce zabawek.
* Renifery zajmują się reniferowymi sprawami.
* Jeżeli elf napotka na problem przy pracy musi prosić Mikołaja o pomoc.
* Renifery po załatwieniu swoich spraw zbierają się w stajni, czekając na rozworzenie prezentów.
* Ponieważ Mikołaj nie lubi, gdy przerywa mu sie drzemkę błahostkami, musi zebrać się odpowiednia liczba elfów 
  (ELF_WAIT_MIN) lub wszystkie renifery (REN_WAIT_MIN=REN_TOTAL), by wstał i zajął się nimi.
* Renifery mają priorytet nad elfami.

## Rozwiązanie

* Mikołaj, renifery i elfy mają swoje zmienne warunkwe na których czekają na odpowiednie wydarzenie:

    + Mikołaj śpi na swojej zmiennej, aż nie zostanie obudzony przez odpowiednią liczbę swojego "personelu" 
      (ELF_WAIT_MIN lub REN_WAIT_MIN)
    + Renifery czekają po wejsciu do swojej "poczekalni", aż Mokołaj zwolni je po rozwiezieniu prezentów
    + Elfy czekają po wejściu do swojej "poczekalni", aż Mikołaj zwolni je po rozwiązaniu ich problemów w fabryce

  Zmiennym warunkowym towarzyszą mutexy potrzebne do ich działania.

* Liczba reniferów i elfów w ich odpowiednich poczekalniach służy do okreslenia, czy należy już budzić Mikołaja. Zmiana
  tych liczb jest sekcją krytyczną, dlatego dla każdej poczekalni zastosowano po jednym mutexie.

* Mikołaj po obudzeniu przechodzi najpierw do reniferów a następnie do elfów. Jeżeli w takiej chwili czeka odpowiednia
  liczba reniferów, to zostaną one obsłużone najpierw, nawet jeżeli to nie one wysłały sygnał, który obudził Mikołaja
  (tj. Mikołaj mógł zostać obudzony przez sygnał elfów, a sygnał reniferów został zignorowany). Takie podejście zapewnia
  priorytet obsługi reniferów.

* Dodatkowo założono, że elfów może czekać więcej, niż minimalna liczba potrzebna do obudzenia Mikołaja
  (brak ograniczenia rozmiaru poczekalni). Efy mogą zgłaszać się do poczekalni nawet jeżeli Mikoła już jest obudzony,
  ale jeszcze nie zaczął obsługiwać elfów z poczekalni. Gdy Mikołaj zaczyna obsługiwać elfy w poczekalni to opuszcza
  mutex odpowiadający za tę poczekalnię – zatem wszyscy "spóźnialscy" elfowie nie mogą się zameldować w poczekalni i
  muszą czekać na kolejną turę.