## Çelj

Çelj - sozdatj igru na konkurs [Ретро вайб — 95](https://gamedev.ru/projects/forum/?id=271231).

Sroki vesjma ograniceny i posemu ne stoit delatj sliškom složnuju igru.
Nado sdelatj cto-to prostoje s tocki zrenija programmirovanija i sozdanija grafiki, pri etom ctoby byla vozmožnostj dorabotatj igru, jesli ostanetsä vremä.

Tematika konkursa predpolagajet pocti cto navernäka dvuhmernuju igru.
Razrešemije ekrana tože budet neboljšim.
Ishodä iz etogo vyhodit, cto vsä grafika budet sostojatj iz neboljših sprajtov (8-16 pikselej).


## Zadumka

Jestj sledujuscaja ideja: sdelatj igru, javläjuscujusä odnovremenno klonom neskoljkih izvestnyh staryh igr, vrode "Arcanoid", "Tetris", "Snake", "Pac-Man", "Mario" i t. d.
Každaja iz etih igr prosta i trebujet nemnogo sil na sozdanije.
Jesli ostanetsä vremä, možno dobavitj jescö igr.

No prosto sdelatj nabor igr - skucno.
Nado sdelatj tak, ctoby odna igra peretekala v druguju.
Eto kak vrode vidišj tu že grafiku, no povedenije menäjetsä na druguju igru, a zatem i grafika.

Podobnyj podhod, kažetsä, možet prevratitj prostoj sbornik igr v cto-to boleje interesnoje.


### Transformaçii

Vozmožny sledujuscije transofrmaçii iz odnoj igry v druguju:
* "Arkanoid" v "tetris". Korablj perehodit v figuru "T".
* "Tetris" v "Snake". Dlinnaja palka (figura "I") stanovitsä zmejoj.
* "Snake" v "Pac-Man". Zmeja stanovitsä "Pac-Man"-om, jeda zmei prevrascajetsä v jedu "Pac-Man"-a.
* "Pac-Man" v "Mario". Geroj prevrascajetsä.


### Smesj igr

Dopolniteljno k transformaçijam možno v posledujuscije igry dobavlätj elementy iz predyduscih igr:

* V "Tetris" inogda vyletajet šarik iz "Arkanoid".
* V "Snake" inogda padajut figury iz "Tetris" i ot nih nado uvoraicivatjsä.
* V "Pac-Man" možno dobavitj bonus v vide šarika iz "Arkanoid", kotoryj by ubival prizrakov.
* V "Pac-Man" možno inogda prevrascatj jedu v jedu iz "Snake", pri podbore kotoroj geroj prevrascalsä by na vremä v zmeju.
* V "Mario" možno sdelatj vraga v forme korablä iz "Arkanoid".
* V "Mario" možno sdelatj pryžki po figuram iz "Tetris". I ctoby oni padali i (vozmožno) povoracivalisj.
* V "Mario" možno dobavitj prizrakov iz "Pac-Man".

Vozožno, takže, stoit nemnogo dobavlätj elementy iz posledujuscih igr v prediduscije.


## Jazyk i nazvanije

Dlä raznoobrazija interesno sdelatj vnutriigrovoj tekst na nemeçkom jazyke.
Kažetsä, dlä prostoj igry eto ne budet problemoj, jesli igrok cego-to ne pojmöt.

Nazvanije tože, stoit sdelatj na nemeçkom jazyke.
Po-horošemu, eto dolžno bytj odno slovo s glubokim smyslom, vozmožno sostavnoje.

Vozmožnyje nazvanija:

* "Vermischung"
* "Quatch"


## Tehniceskaja storona voprosa

Prostuju igru prosce delatj bez storonnih dvižkov.
Maksimum - cto-to vrode biblioteki "SDL2".
Apparatnoje uskorenije tože ne nužno, hvatit i programmnogo risovanija.

Pisatj možno na "C++" ili na "Rust.


## Grafika

Stoit risovatj vsö v bufer fiksirovannogo razmera, kak na staryh videoadapterah, s çelocislennym masštabirovanijem.
Tehniceski pri etom (dlä prostoty) vnutri možno rabotatj s 32-bitnym çvetom, no resursy budut sozdany v ogranicennoj palitre.

Resursy stoit risovatj v palitre iz 16 ili 64 çvetov.


## Zvuk

Zvuk stoit imetj v nizkom razrešenii i s zakosom pod cto-to vrode "PC-Speaker".
Pri etom vyvoditj zvuk možno kak obycno eto delajetsä v "SDL2".
