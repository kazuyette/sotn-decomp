# Journal de bord — sotn-decomp

Journal des découvertes et de l'état d'avancement du projet, tenu au fil des sessions.

## 2026-07-15 — Mise en place du repo

- Clone de [Xeeynamo/sotn-decomp](https://github.com/Xeeynamo/sotn-decomp) copié dans ce dossier.
- L'historique git complet (`.git/`, ~400 Mo) n'a pas pu être copié de façon fiable sur ce
  volume : seul l'arbre de travail (fichiers sources) est présent. Pour cloner/committer/pousser,
  refaire un `git clone` directement sur le disque local plutôt que via cet environnement.
- `tools/psyz/external/SDL` (dépendance de build de l'outil `psyz`, un émulateur/debugger PS1
  auxiliaire, non nécessaire au travail de decomp lui-même) a été récupéré intégralement.

## 2026-07-15 — Identification du disque fourni

Le fichier `Akumajo Dracula X - Gekka no Yasoukyoku (JAP) (DW0007).bin` (+ `.cue`), malgré son nom
laissant penser à un disque PS1 japonais, est en réalité une image de disque **Sega Saturn** :

```
SEGA SEGASATURN SEGA TP T-95    T-9527G   V1.40019980427CD-1/1  J J DRACULA-X
```

- Numéro de série Saturn : **T-9527G**
- Date de build : 1998-04-27
- Titre : *Akumajo Dracula X: Gekka no Yasoukyoku* — le portage Saturn de *Symphony of the Night*,
  réalisé par une équipe externe (Aisystem Tokyo), avec du contenu bonus (mode dessin, boss
  supplémentaires Regeneratio/Iron Golem, palette graphique modifiée).
- Correspond exactement à la cible `saturn` déjà supportée par `sotn-decomp`
  (`config/check.saturn.sha`, `config/saturn/*.prg.yaml`, `Makefile.saturn.mk`).

### Extraction

`bchunk` et `7z` ne sont pas installables dans cet environnement (pas de droits root/sudo, apt
bloqué). Extraction faite manuellement en Python :

1. Conversion des secteurs bruts (Mode1/2352, 204 663 secteurs pour la piste de données) en ISO
   2048 octets/secteur — script `bin2iso.py` (offset 16, longueur 2048 par secteur).
2. Lecture du système de fichiers ISO9660 avec `pycdlib`.
3. Extraction des 437 fichiers vers `disks/saturn/`.

### Vérification

Les 8 fichiers listés dans `config/check.saturn.sha` correspondent **exactement** (SHA1) aux
fichiers extraits :

| Fichier | SHA1 attendu | Statut |
|---|---|---|
| GAME.PRG | f03234c4f2... | ✅ |
| T_BAT.PRG | d513a3a1fc... | ✅ |
| 0.BIN | 54c2e3e59f... | ✅ |
| STAGE_02.PRG | f44004e15f... | ✅ |
| WARP.PRG | ebc09d976d... | ✅ |
| ALUCARD.PRG | 8024fbf536... | ✅ |
| RICHTER.PRG | 9d3a5a0676... | ✅ |
| MARIA.PRG | edd410154a... | ✅ |

→ C'est bien le disque de référence attendu par le repo pour la cible `saturn`.

## 2026-07-15 — Limites de l'environnement Cowork pour le build

Cet environnement sandbox ne permet pas (pas de root, réseau restreint à une allowlist) :

- l'installation de **Go** (nécessaire à `tools/sotn-assets`, `tools/sotn-disk`)
- l'installation de **Rust/Cargo** (`rustup.rs` bloqué par l'allowlist réseau) — nécessaire à
  `tools/saturn-splitter` (splitter SH2, en cours de dev) et `tools/saturn-splitter/adpcm-extract`
  (conversion PCM→WAV des pistes audio)
- l'installation de **sh-elf-gcc/binutils** et **Docker** — nécessaires pour recompiler et diffs
  le binaire Saturn (le splitter Saturn documente l'usage de deux containers Docker : un avec
  sh-elf-gcc/binutils, l'autre avec `dosemu` pour le compilateur Cygnus DOS d'origine)
- `apt-get install` (bchunk, p7zip, cmake, ninja) — bloqué, pas de droits root

**Conclusion pratique** : la génération de code assembleur/C réel (split, compilation, diff) pour
la version Saturn devra se faire sur ta machine locale, où ces toolchains peuvent être installées
normalement. Cet environnement reste utile pour : lire/analyser le code déjà décompilé, préparer
les assets, écrire/relire du C, et faire de la reconnaissance sur les fichiers du disque.

## 2026-07-15 — Contournement : désassemblage SH2 sans toolchain

`capstone` (bibliothèque Python, `pip install capstone`) supporte nativement l'architecture SH
(`CS_ARCH_SH`, mode `CS_MODE_SH2`). Ça permet de lire le code SH2 des `.PRG` Saturn **sans**
sh-elf-objdump/Docker — utile pour l'analyse manuelle en attendant l'installation des vraies
toolchains en local.

Outil ajouté : `tools/sh2_disasm.py` — lit `config/saturn/<name>.prg.yaml` pour connaître l'adresse
VRAM et les sous-segments (code vs data) d'un `.PRG`, puis désassemble. Usage :

```
python3 tools/sh2_disasm.py alucard --end 0x120
```

⚠️ Limites : pas de résolution de symboles, pas de détection fiable des frontières de fonctions au
delà des heuristiques déjà présentes dans les yaml, sortie non réinjectable dans le build. Un outil
de lecture, pas un remplaçant du vrai splitter.

### Première fonction repérée

`ALUCARD.PRG`, offset `0x60`–`0xCA` (VRAM `0x060A5060`–`0x060A50CA`), aucun symbole existant dans
`alucard_user_syms.txt` (qui ne contient que `_g_Entities`). Prologue/épilogue standards
(`mov.l r14,@-r15` / `mov r15,r14` ... `mov r14,r15` / `rts` / `mov.l @r15+,r14`), corps qui lit des
bits de flags sur une structure pointée par `r6` (probablement liée à `_g_Entities`) et renvoie
-1/0/1 selon les combinaisons — a priori une fonction d'état/vérification tri-valuée. Bon candidat
comme toute première fonction à décompiler à la main côté Saturn.

## 2026-07-15 — Vérification du disque US (SLUS-00067)

Le dossier `SLUS-00067/` fourni contient 2 pistes bin (`Track 1` = données MODE2/2352, `Track 2` =
audio séparée) + `.cue`. Même méthode que pour le disque Saturn :

1. Extraction de la piste de données en ISO 2048 octets/secteur (Mode2 Form1, offset 24, longueur
   2048 par secteur de 2352 — 229 020 secteurs, taille exacte, aucune perte).
2. Lecture ISO9660 avec `pycdlib` → 296 fichiers (`SLUS_000.67`, `SYSTEM.CNF`, `DRA.BIN`, `BOSS/`,
   `BIN/`, `ST/`, `SERVANT/`, `SD/XA_STR1.`, etc.), structure conforme à `config/disk.us.lba`.
3. Extraction complète vers `disks/us/` (448 Mo, le plus gros fichier étant `SD/XA_STR1.` à 371 Mo
   — copié par blocs de 40 Mo puis réassemblé côté destination, une reprise a été nécessaire après
   une coupure à mi-écriture mais l'intégrité a été revérifiée par hash).

### Vérification

Les 77 entrées de `config/check.us.sha` correspondent **toutes** exactement (SHA1) aux fichiers
extraits (`main.exe` → `SLUS_000.67`, `DRA.BIN`, tous les `BOSS/BOx/*.BIN`, `ST/*/*.BIN`, etc.).
Une entrée (`CHI.BIN`) semblait différer à cause d'une casse mixte dans le fichier `check.us.sha`
du repo lui-même (hash hexadécimal avec des majuscules) — pas une vraie erreur, la comparaison
insensible à la casse confirme la correspondance.

→ Disque US authentique et intact, `disks/us/` prêt pour `./sotn.sh extract us` /
  `make build_us` (une fois les toolchains Go/Python installées).

Reliquat inoffensif : `disks/us/SD_split/` (fragments temporaires de la copie de `XA_STR1.`) n'a
pas pu être supprimé sur ce volume (même limitation que le dossier `sotn-decomp/` vide mentionné
plus haut) — à ignorer, aucun impact sur le contenu réel.

## Ressources externes

- [SOTN Decomp Wiki](https://mathisto.github.io/sotn-wiki/) — wiki communautaire qui relie lore/
  connaissance du jeu (stages, ennemis, boss, objets, mécaniques) directement au code décompilé.
  Section [Decomp Docs](https://mathisto.github.io/sotn-wiki/decomp/) utile pour la doc technique.
  Autres liens utiles listés là-bas : [sotn.xee.dev](https://sotn.xee.dev/) (progression temps
  réel), [decomp.me](https://decomp.me/) (decomp collaboratif).
- [decomp.dev/Xeeynamo/sotn-decomp](https://decomp.dev/Xeeynamo/sotn-decomp) — tracker de
  progression **actuel/officiel**, mis à jour à chaque commit (40,83% décompilé au total, toutes
  versions, au 2026-07-15). Plus à jour que `sotn.xee.dev` ci-dessous.
- [sotn.xee.dev](https://sotn.xee.dev/) — tracker de progression par fichier/overlay (% par version),
  lu via l'extension Chrome. Voir tableau ci-dessous.
- [decomp.wiki](https://decomp.wiki/) (Decompedia) — hub généraliste sur la décompilation de jeux
  vidéo (pas spécifique à SOTN), pointe vers `decomp.me`, `decomp.dev`, `ghidra.decomp.dev`. Utile en
  contexte/méthodologie, rien de directement actionnable pour ce projet.

### Progression US au 2026-07-15 (extrait de sotn.xee.dev)

Le moteur principal est déjà quasi complet côté US : `dra` 100%, `ric` 100%, `main` 98%, `sel` 97%,
la plupart des stages normales à 100%. Les plus gros manques sont du côté **boss** et **reverse** :

| Zone | Statut US |
|---|---|
| bo1, bo2, bo3, bo5, bo7 | 0% (rien fait) |
| bo6 | 46% |
| bo0 | 58% |
| rbo1, rbo2, rbo4, rbo6, rbo7, rbo8 | 0% |
| rchi, rdai, rlib, rno1, rno2, rno4, rnz1 | 0% |
| tt_005, tt_006 | 0% |

→ Les boss (`bo1`–`bo7`) sont clairement la zone la plus rentable pour une contribution US : gros
morceaux à 0%, contrairement au moteur principal déjà quasi terminé.

## 2026-07-15 — `splat` fonctionne réellement dans cet environnement (côté US)

Contrairement à Saturn, le vrai outil du projet pour spliter le binaire US (`splat` / `splat64`,
Python pur — pas besoin de Go/Rust/mipsel-gcc pour cette étape) **tourne dans ce sandbox**. Il
manquait juste des dépendances pip non listées explicitement comme nécessitant un compilateur :

```
pip install splat64==0.34.1 spimdisasm==1.35.0 rabbitizer==1.13.0 mapfile-parser==2.1.4 \
    crunch64 n64img==0.3.3 pygfxd cxxfilt tabulate intervaltree==3.1.0 pillow toml \
    --break-system-packages
```

Test réussi : `python3 -m splat split config/splat.us.main.yaml` depuis la racine du repo a
correctement régénéré `asm/us/main/` (asm désassemblé, matchings, linker script) en utilisant les
symboles déjà connus (`config/symbols.us*.txt`) — 173 fichiers `c` et 35 `asm` reconnus, seulement
43 fonctions non matchées restantes dans `main`, toutes dans `psxsdk` (bibliothèque BIOS/CD, pas de
la logique de jeu).

**Ce qui manque encore pour un build complet** (compiler + vérifier le matching octet-pour-octet) :
`mipsel-linux-gnu-gcc/as/ld/objcopy` (cross-toolchain PSX), **Go** (`tools/sotn-assets`,
`tools/sotn-disk` — packing des assets/disque), **Rust** (`tools/mipsmatch`). Ces trois-là restent
bloqués ici (pas de root/apt, pas de cargo/go, réseau allowlisté) — à installer en local si tu veux
un build qui compile et diff réellement le binaire.

**Conclusion pratique** : pour lire l'assembleur MIPS existant, comparer avec le C déjà écrit,
identifier les fonctions non matchées et écrire du C candidat (le cœur du travail quotidien de
decomp), cet environnement suffit déjà côté US — pas besoin d'attendre l'installation locale.

## 2026-07-15 — Ampleur du travail sur un boss à 0% (test sur BO1 / Granfaloon)

Contrairement à `bo0`/`bo4`/`bo6` (qui ont déjà un `config/splat.us.boboN.yaml`), **aucun config
n'existe** pour `bo1`, `bo2`, `bo3`, `bo5`, `bo7` — ce ne sont pas juste des fonctions non matchées
comme dans `main`, c'est un split jamais commencé.

Test : config minimal "à plat" pour `BO1.BIN` (205 756 octets, tous à 0x80180000 comme les autres
boss — même slot d'overlay réutilisé) avec un seul segment `type: c` couvrant tout le fichier et
`find_file_boundaries`/`disassemble_all` activés, en espérant que `splat` détecte seul les
fonctions. Résultat : **un seul "bloc"** de 56 544 lignes d'asm, en grande partie des instructions
`/* handwritten instruction */` — c'est-à-dire des octets de `.data`/`.rodata` mal interprétés comme
du code MIPS, parce que rien n'indique à `splat` où s'arrête la donnée et où commence le code.

C'est exactement le travail qu'on voit déjà fait à la main dans `config/splat.us.bobo0.yaml` : ~90
lignes de sous-segments `data`/`.rodata`/`c` positionnés et nommés un par un (`e_lock_camera`,
`e_breakable`, `st_update`, `st_collision`, `e_collect`, `popup`, `prim_helpers`, etc. — des
fonctions/données *partagées* entre tous les boss/stages, reconnaissables en comparant avec les
overlays déjà splittés).

**Conclusion** : attaquer un boss à 0% n'est pas "trouver une fonction à décompiler", c'est d'abord
refaire ce travail de repérage de frontières — repérer dans `BO1.BIN` les mêmes fonctions communes
déjà identifiées dans `bo0`/`bo4`/`bo6` (souvent by cross-référence d'octets identiques ou très
proches, un peu comme `mipsmatch` est censé le faire, mais `mipsmatch` est en Rust et indisponible
ici), avant de pouvoir produire un yaml exploitable. Nettement plus long qu'une fonction isolée dans
`main`, mais aussi plus structurant : une fois `bo1` découpé, `bo2`/`bo3`/`bo5`/`bo7` iront plus vite
par comparaison (beaucoup de code est probablement partagé entre boss).

## 2026-07-15 — Vérification du disque PSPEU

`PSPEU/Castlevania - The Dracula X Chronicles (Europe) (...).iso` est un ISO PSP UMD standard
(2048 octets/secteur, pas de conversion nécessaire, contrairement aux disques PS1/Saturn). Ouverture
directe avec `pycdlib`, structure conforme à l'attendu : `PSP_GAME/USRDIR/res/ps/PS.ELF` +
`PSP_GAME/USRDIR/res/ps/PSPBIN/*.bin` (chaque overlay du jeu original PS1 recompilé, dont
`bo1.bin`…`bo7.bin` — potentiellement utile en référence croisée même si non encore splittés).

Vérification : `PS.ELF` (`main.exe` dans `check.pspeu.sha`) + les 34 autres entrées (`dra.bin`,
`ric.bin`, `are.bin`, etc.) → **35/35 hashs corrects**. Disque authentique.

Pas encore extrait dans `disks/pspeu/` (1,1 Go, plus gros que US/Saturn) — à faire si besoin, pour
l'instant seule la vérification a été faite (lecture directe depuis l'ISO, rien copié).

## 2026-07-15 — Tentative de repérage de frontières pour `bo1` par comparaison d'octets

Idée testée : extraire les 39 fonctions déjà nommées de type `c` dans `bo0`/`bo4`/`bo6` (via leurs
yaml) et chercher ces séquences d'octets telles quelles dans `BO1.BIN`/`BO2.BIN`/`BO3.BIN`/
`BO5.BIN`/`BO7.BIN`, en pariant que le code partagé (`st_update`, `e_collect`, `popup`,
`prim_helpers`...) est recompilé à l'identique.

**Résultat : 0 correspondance exacte sur les 5 fichiers.** Attendu après coup : le code MIPS compilé
contient des adresses absolues (`jal` vers d'autres fonctions, `lui`/`ori` pour charger des adresses
de données) qui dépendent de l'endroit où tout est placé dans *ce* fichier précis. Une fonction
partagée peut être logiquement identique d'un boss à l'autre sans être identique **en octets bruts**,
dès que les fonctions/données qu'elle référence ne sont pas aux mêmes offsets relatifs.

→ C'est exactement le problème que `tools/mipsmatch` (Rust, non buildable ici, cf. plus haut) est
conçu pour résoudre : il normalise/masque les champs d'adresse avant de comparer. Une comparaison
"à la main" avec `asm-differ` (désassemblage côte à côte, en ignorant les opérandes d'adresse) reste
possible ici en Python, mais fonction par fonction plutôt qu'en recherche automatique en masse.
Le repérage de frontières sur `bo1` demandera donc soit `mipsmatch` en local, soit un travail plus
manuel via désassemblage comparé.

## 2026-07-15 — Toolchain locale installée (WSL2 Ubuntu 24.04) + premier vrai split `bo1`

Installation complète faite par l'utilisateur en local, dans WSL2 (pas Windows natif — le Makefile
suppose bash/wget/tar/snap) :

- Rust (`rustup default stable`) + Go (`snap install --classic go`, penser à ajouter `/snap/bin` au
  `PATH` — piège rencontré : nouveau terminal WSL n'hérite pas forcément du `PATH` mis à jour par un
  script d'install lancé dans une session précédente).
- `make update-dependencies` (télécharge le compilo PSX précompilé `cc1-psx-26` et les autres outils).
- Piège : le dossier de travail était sur `/mnt/e/...` (disque Windows monté dans WSL) — `python3 -m
  venv` échouait silencieusement dessus (`ensurepip` réussissait en fait, mais recréer/déplacer les
  scripts `pip`/`pip3` échouait avec `Operation not permitted`, exactement le même genre de souci
  rencontré plus haut avec l'écriture sur le volume Cowork). **Un vrai `git clone` dans le filesystem
  Linux natif (`~/sotn-decomp`, pas `/mnt/e/...`) a réglé le problème d'un coup.** Les gros fichiers
  (disques) peuvent être lus depuis `/mnt/e/...` sans souci, seule l'écriture y est pénible.

Build complet réussi : `make extract_disk && make extract -j && make build -j` → `✅ us` (le check
octet-pour-octet intégré au build passe). `make check` n'existe pas comme cible séparée — c'est ce
`✅` affiché après le build qui fait office de vérification, l'instruction du wiki à ce sujet est
obsolète/erronée.

### `mipsmatch` construit et utilisé avec succès sur `bo1`

`cargo build --release` dans `tools/mipsmatch` (ou `make bin/mipsmatch`) — aucun souci de
compilation. Avec `build/us/bobo0.elf`/`.map`, `bobo4`, `bobo6` (produits par le build ci-dessus) :

```
bin/mipsmatch --output build/us/match.boboN.yaml fingerprint build/us/boboN.map build/us/boboN.elf
bin/mipsmatch scan build/us/match.boboN.yaml disks/us/BOSS/BO1/BO1.BIN
```

**15 segments partagés retrouvés dans `BO1.BIN`** (mêmes offsets peu importe la référence bo0/bo4/
bo6 utilisée, cohérent) : `e_room_bg`, `e_lock_camera`, `st_update`, `st_collision`, `create_entity`,
`e_red_door`, `st_common`, `e_misc`, `e_collect`, `e_stage_name`, `e_particles`, `e_room_fg`, `popup`,
`prim_helpers`, `e_life_up` — 105 symboles de fonctions nommés au total.

→ **`config/splat.us.bobo1.yaml` et `config/symbols.us.bobo1.txt` créés** à partir de ces résultats
(offsets file → VRAM = offset + 0x80180000). Testé avec succès (`python3 -m splat split
config/splat.us.bobo1.yaml`) : 24,37% de `bo1` splitté proprement en 15 fichiers `.c` nommés et
lisibles (ex: `Random`/`Update`/`HitDetection`/`CreateEntityFromLayout`), symboles globaux résolus
correctement (`g_randomNext`, etc.).

Reste ~155 Ko non identifiés (spécifique à Granfaloon, le boss lui-même) en 3 zones marquées `data`/
TODO dans le yaml :
- `0x0`–`0x20CDC` (~133 Ko, avant le premier segment partagé)
- `0x20F60`–`0x255A0` (~18 Ko, entre `e_lock_camera` et `st_update`)
- `0x31700`–fin (~3,2 Ko, queue du fichier)

Indice bonus donné par `splat` : une jump table détectée à VRAM `0x801A0B98` (offset `0x20B98`)
suggère que la `.rodata` de Granfaloon commence peu avant ce point — utile pour attaquer la première
zone `data` à la main.

→ Une fois `bo1` entièrement splitté, `bo2`/`bo3`/`bo5`/`bo7` devraient aller plus vite : même
méthode `mipsmatch` (déjà éprouvée), et une partie du code boss-spécifique pourrait aussi être
partagée entre plusieurs de ces boss (à vérifier).

## 2026-07-15 — `mipsmatch` répété sur `bo2`/`bo3`/`bo5`/`bo7`

Même méthode que `bo1`, avec les mêmes fichiers de référence (`match.bobo0/4/6.yaml`). Résultats
(après dédoublonnage des 3 scans, identiques à chaque fois) :

| Boss | Segments trouvés | Couverture | Particularité |
|---|---|---|---|
| bo1 (Granfaloon) | 15 | 24,4% | — |
| bo2 (Minotaur & Werewolf) | 16 | 22,6% | seul à avoir `e_breakable` en plus |
| bo3 (Scylla) | 14 | 18,8% | ni `e_collect` ni `e_breakable` — probablement pas de drop d'objet dans ce combat |
| bo5 (Hippogryph) | 15 | 22,9% | — |
| bo7 (Cerberus) | 14 | 34,6% | pas de `e_room_bg` ; fichier le plus petit (144 Ko) donc le code partagé y pèse proportionnellement plus |

`config/splat.us.bobo{2,3,5,7}.yaml` + `config/symbols.us.bobo{2,3,5,7}.txt` créés sur le même
modèle que `bo1` (zones `data`/TODO pour le code encore spécifique à chaque boss) et testés avec
succès (`python3 -m splat split ...`, 0 erreur pour les 4).

→ Les 5 boss (`bo1`, `bo2`, `bo3`, `bo5`, `bo7`) ont maintenant un point de départ réel. Le code
spécifique à chaque boss (Granfaloon, Minotaur/Werewolf, Scylla, Hippogryph, Cerberus) reste à
décompiler à la main — c'est là qu'est la vraie valeur ajoutée maintenant, le reste était du code
partagé déjà connu ailleurs dans le projet.

## Prochaines étapes

- [x] Récupérer le disque US (SLUS-00067) — déposé dans `SLUS-00067/` (2 pistes bin + cue).
- [x] Extraire et vérifier le disque US contre `config/check.us.sha` — 77/77 OK, voir section
      dédiée ci-dessus. `disks/us/` peuplé.
- [x] Faire tourner `splat` côté US — fonctionne, voir section dédiée.
- [x] Choisir un boss à 0% et sonder l'étendue du travail — fait sur `bo1`, voir section dédiée :
      aucun config n'existe, le split doit être fait à la main en repérant les fonctions/données
      partagées avec `bo0`/`bo4`/`bo6`.
- [x] Comparer `BO1.BIN` aux fonctions communes de `bo0`/`bo4`/`bo6` — fait avec `mipsmatch` en
      local (toolchain WSL installée), 15 segments/105 symboles trouvés, `config/splat.us.bobo1.yaml`
      + `config/symbols.us.bobo1.txt` créés et testés (24,37% splitté). Voir section dédiée.
- [x] Répéter `mipsmatch` sur `bo2`/`bo3`/`bo5`/`bo7` — fait, 4 nouveaux yaml/symbols créés et
      testés. Voir section dédiée.
- [x] Analyser les 3 zones `data`/TODO de `bo1` (scan `jr $ra`) — fait. Zone 1 (`0x0`-`0x20CDC`,
      ~133 Ko) et zone 3 (`0x31700`-fin, ~3,2 Ko) : 0 retour de fonction trouvé → données pures
      (tables de layout/animations), confirmé aussi par un indice de jump table à `0x20B98` et un
      indice de rodata à `0x20AA0` détectés par `splat`, tous deux dans la zone 1. Zone 2
      (`0x20F60`-`0x255A0`, ~18 Ko, entre `e_lock_camera` et `st_update`) : **18** retours de
      fonction trouvés → vrai code, testé en `type: c` dans `splat.us.bobo1.yaml` → split propre
      en 18 fonctions (`func_us_801A0F60` à `func_us_801A518C`) dans `src/boss/bo1/20F60.c`.
- [x] Décompiler à la main les 4 premières fonctions de `20F60.c` (`func_us_801A0F60`,
      `func_us_801A10A8` — effet de débris de Granfaloon —, `func_us_801A1334` — machine à états du
      corps principal —, `func_us_801A17F8` — utilitaire générique) — fait via m2c + retypage manuel
      (champs `Entity`/`Primitive` nommés, structs `ET_<VRAM>` pour les régions `ext` réutilisées),
      puis **vérifié par une vraie compilation WSL** (`make build -j` → `✅ us`, ROM identique
      octet pour octet à `config/check.us.sha`). C'est la première fois que du code boss-spécifique
      de ce projet est prouvé correct plutôt que simplement plausible.
- [x] Décompiler les 8 fonctions suivantes de `20F60.c` — voir section dédiée ci-dessous
      (2026-07-15, batch de 14).
- [ ] Décompiler les 6 dernières fonctions de `20F60.c` (`func_us_801A1878`, `func_us_801A2F2C`,
      `func_us_801A3480`, `func_us_801A38EC`, `func_us_801A4AF4`, `func_us_801A518C`) — ce sont les
      plus grosses/les plus neuves (machines à états à jump table de 300-1000+ lignes, arithmétique
      de pointeurs entre segments de chaîne, rendu Primitive dense) ; reportées à une prochaine
      session plutôt que bâclées.
- [ ] Faire tourner `asm-differ`/`make build -j` en WSL sur les 8 nouvelles fonctions pour confirmer
      l'octet-exactitude (comme pour les 4 premières).

## 2026-07-15 — `bo1` : 8 fonctions supplémentaires de `20F60.c` (batch de 14)

Suite de la décompilation à la main de `src/boss/bo1/20F60.c`. Sur les 14 fonctions restantes de
la zone 2 (gap2), 8 ont été entièrement décompilées et retypées cette session ; les 6 plus grosses
sont reportées (voir "Prochaines étapes"). Méthode identique aux 4 premières : m2c (avec un
contexte réutilisable construit à partir d'une version entièrement stubée du fichier, pour éviter
un bug de m2c qui plante sur le contexte quand le fichier contient déjà du vrai code retypé),
retypage manuel via les champs `Entity`/`Primitive` confirmés, structs `ET_<VRAM>` pour les régions
`ext`, TODO honnêtes sur ce qui reste incertain.

Fonctions traitées : `func_us_801A2774`, `func_us_801A2BC4`, `func_us_801A2CC4`, `func_us_801A2D90`,
`func_us_801A4394`, `func_us_801A45D0`, `func_us_801A493C`, `func_us_801A50BC`.

Découvertes notables :

- **Table partagée `D_us_80180D34`** (stride `0xA`, 5 champs `u16`) : ses limites de champs ont pu
  être confirmées en croisant 3 fonctions consommatrices différentes, chacune lisant un sous-
  ensemble d'offsets différent (`func_us_801A2774` lit les offsets `0x0`/`0x2`, `func_us_801A2BC4`
  lit `0x4`/`0x6`, `func_us_801A2CC4` lit `0x8` — ce dernier offset, que m2c avait nommé
  `D_us_80180D3C` séparément, n'est en fait que la même table + 8 octets, confirmé par l'arithmétique
  d'adresses (`0x80180D3C - 0x80180D34 == 8`, exactement le stride).
- **Entité type `0x1E` = `func_us_801A2CC4`** : confirmé en croisant le site d'allocation dans
  `func_us_801A2774` (qui écrit le pointeur `ext+0x9C` de l'entité `0x1E` fraîchement créée vers
  `self`) avec `func_us_801A2CC4` (qui lit exactement ce même pointeur/offset pour suivre la
  position d'une entité cible chaque frame). Un des rares cas où la fonction "propriétaire" d'un
  type d'entité a pu être identifiée avec certitude sans avoir décompilé sa définition elle-même.
- Champ non nommé `flags & 0x100` vu dans 3 fonctions différentes (`2774`, `4394`, `45D0`), toujours
  déclenché par le même bit `D_us_80180CE4 & 1` ("fin d'attaque" partagée) — probablement un flag
  "en cours d'atterrissage/plantage" commun à toutes les attaques au sol de Granfaloon.
- `Collider` (déjà défini dans `include/game.h`, champ `unk18` commenté "top edge of queried tile
  collision") s'est révélé être exactement le champ de correction utilisé par
  `func_us_801A45D0` pour recaler `posY` après contact au sol — déduit d'un stack slot que m2c
  n'arrivait pas à résoudre (`sp28`), à un offset qui correspond pile à `Collider.unk18`.

Confiance : haute sur le flux de contrôle et les champs `Entity` nommés (tous confirmés par
recoupement). Plus basse sur : la disposition exacte de la table `D_us_80180E34` (gardée en
arithmétique d'offsets bruts plutôt qu'une struct devinée à tort), le vrai sens du byte `ext+0xA`
du parent dans `func_us_801A2774` (`0x86`), et les 3 champs `ext` écrits par `func_us_801A50BC`
mais jamais lus dans ce fichier (probablement consommés par la fonction de mise à jour du type
d'entité `0x22`, pas encore décompilée). **Pas encore vérifié par une vraie compilation WSL** — lint
syntaxique fait dans ce sandbox (gcc -fsyntax-only), mais l'octet-exactitude reste à confirmer.
      en 18 fonctions (`func_us_801A0F60` à `func_us_801A518C`), désassemblage MIPS valide (aucun
      "handwritten instruction" garbage, ex. `func_us_801A10A8` appelle `InitializeEntity` avec un
      prologue standard). Couverture `bo1` : 24,37% → **33,11%**. `splat.us.bobo1.yaml` mis à jour
      (zone 2 en `c`, zones 1/3 restent `data` avec commentaires à jour).
- [x] Première fonction de la zone 2 de `bo1` décompilée : `func_us_801A0F60`. `m2c` installé
      (`tools/m2c` du repo + `tools/decompile.py`), splitté et passé sur la fonction, puis
      retypée à la main avec le struct `Entity` déjà nommé dans `include/game.h` — `arg0` est un
      `Entity*`, et les offsets `unk2C/unk30/unk44/unk18/unk47/unk54/unk24` correspondent
      exactement à `step/params/hitParams/blendMode/hitboxHeight/animSet/zPriority`. Résultat :
      la fonction anime l'entité (attaque en cours via `params>>0xC` comme index), joue un son et
      fait apparaître un item si `hitParams != 0` (mort du breakable), sinon initialise l'entité
      via des tables `D_us_80180C4x/5x/6x` (propres à Granfaloon, situées dans la zone `data`
      encore non identifiée — cohérent). Fichier `src/boss/bo1/20F60.c` mis à jour dans le repo,
      les 17 autres fonctions de la zone restent en `INCLUDE_ASM` (non traitées).
  - **Bug/incohérence trouvée dans `splat` (v0.34.1, pip)** : pour `bobo1`, les `INCLUDE_ASM`
    générés utilisent le chemin complet `asm/us/boss/bo1/nonmatchings/...` alors que les fichiers
    déjà committés (`bo0` par ex.) utilisent `boss/bo1/nonmatchings/...` (sans le préfixe
    `asm/us/`). `tools/decompile.py` attend la 2e forme (sans préfixe) pour retrouver la fonction
    dans le `.c` — sinon il répond à tort "already decompiled". Si tu relances `splat split`
    toi-même sur un boss et que `decompile.py` dit ça alors que la fonction n'est pas encore
    traitée, vérifie/corrige ce préfixe dans le `.c` généré.
  - **Bug Python 3.10 dans `tools/decompile.py`** : le script utilise des f-strings avec guillemets
    imbriqués (syntaxe Python 3.12+), donc il ne parse même pas sous Python 3.10/3.11. Je l'ai
    corrigé dans mon environnement de test (pas encore reporté dans le repo) — à corriger/PR si
    utile pour les contributeurs qui n'ont pas Python 3.12.
- [x] Deuxième fonction décompilée : `func_us_801A10A8` — l'effet "Granfaloon se brise en débris".
      Bien plus complexe que la première : `self->step==0` alloue une chaîne de `Primitive` (via
      `g_api_func_800EDB58`, encore non nommée) et réutilise leurs octets de couleur normalement
      inutilisés (`0x10-0x13` et `0x14-0x17`) comme accumulateur position 32 bits par débris ;
      `step==1` (ou la suite immédiate de `step==0`) fait avancer la physique de chaque débris
      (vitesse constante en X, gravité qui incrémente la vitesse Y de `0x1000`/frame), les dessine
      relatifs à la position du boss, puis un timer (`self->ext`, init à `0x40` = 64 frames)
      décompte jusqu'à l'auto-destruction (`DestroyEntity`). Confirmé via le flag déjà nommé
      `FLAG_HAS_PRIMS` (`0x800000`, `include/game.h:411`, "call FreePrimitives on DestroyEntity")
      et la convention déjà établie `entity->posX.i.hi` (vue dans `src/dra/4B758.c`) pour extraire
      la partie entière d'un `f32` (qui est en fait un union 16.16 fixed-point, pas un vrai float —
      voir `include/types.h:83`).
  - **Confiance** : haute sur le flux de contrôle et la physique (confirmés par des noms/conventions
    déjà établis ailleurs dans le repo). Plus incertain : la sémantique exacte des tables
    `D_us_80180C74`/`C78` (supposées vitesseX/vitesseY, non vérifiées), et les deux arguments de
    `g_api_func_800EDB58` (`0x11`, `8`). **Non vérifié par compilation/`asm-differ`** — pas de
    toolchain PS1 dans ce sandbox. Vérifié uniquement par un lint syntaxique (`gcc -fsyntax-only`
    contre des stubs de types locaux) : les deux fonctions compilent sans erreur ni warning
    (à part un cast pointeur→int corrigé). **À faire avant de faire confiance à 100%** : lancer
    `asm-differ -mwo --overlay bo1 func_us_801A10A8` dans ta session WSL pour voir le vrai
    pourcentage de match.
  - Nouveau struct `ET_801A10A8` (champs `prim`/`timer` à `0x7C`/`0x80`) déclaré localement dans
    `20F60.c`, pas encore remonté dans `include/entity.h` (convention du repo pour `bo4/doors.c`
    range ce genre de struct dans l'union `Ext` globale — à faire si on formalise ce fichier).
  - **Point méthodologique important** : dès qu'une fonction est retypée avec des types réels
    (`Entity*` au lieu de `void*`), relancer `tools/decompile.py` sur une AUTRE fonction du même
    fichier échoue (`m2ctx`/`m2c` ne retrouve plus le typedef `Entity` dans son contexte reconstruit
    — bug de l'outil, pas du code). Contournement utilisé : revert temporairement la fonction déjà
    retypée en simple `INCLUDE_ASM` stub, lancer `decompile.py` sur la fonction suivante, réappliquer
    ensuite la version retypée à la main. À refaire pareil pour les 16 fonctions restantes.
- [x] Troisième fonction décompilée : `func_us_801A1334` — la machine à états du corps principal
      de Granfaloon (7 états, `self->step` 0-6). Chute depuis le plafond → collision avec le sol
      → rebond/amortissement → écriture d'un motif de tuiles dans `g_Tilemap` (probablement un
      effet de tremblement d'écran/poussière). Contient un `switch` piloté par une vraie jumptable
      (`jtbl_us_801A0AA0`, VRAM `0x801A0AA0`, dans la zone `data` non identifiée — confirme que
      cette zone contient des données *utilisées* par le code, pas des données orphelines).
  - **Découverte utile** : `splat` avait déjà repéré et extrait cette jumptable dans
    `asm/us/boss/bo1/data/0.data.s` (adresses en hexa brut, pas en labels). `m2c` refuse de
    décompiler un switch sans jumptable *symbolique* dans le même fichier — il a fallu ajouter les
    labels `.Lus_<adresse>` manquants dans une copie du `.s` de la fonction + réécrire les entrées
    `.word` du fichier data en labels symboliques avant de les passer ensemble à `m2c`. Procédure
    répétable si d'autres fonctions à switch/jumptable se présentent.
  - Juste après cette jumptable, à VRAM `0x801A0ABC` (donc bien dans la zone `data`), il y a une
    vraie chaîne de debug : `"y %x\n"` — cohérent avec le thème "chute/position Y" de cette
    fonction, further evidence que la zone 1 est bien des données légitimes.
  - Champs réutilisés confirmés via `include/game.h` : `g_Tilemap.scrollX.i.hi`/`scrollY.i.hi`
    (le scroll caméra, offset `0xA`/`0xE` dans `Tilemap`) et `g_Tilemap.fg` (tableau de tuiles,
    premier champ du struct) — pas des suppositions, lus directement depuis le header existant.
  - **Confiance** : haute sur le flux de contrôle et la physique (chute/collision/rebond/spawn de
    débris). Plus incertain : la table `D_us_80180CB4` (typée `s16[2][4]`, indexée directement par
    `self->params` — ne marche que si `params` vaut bien 0 ou 1 ici, alors qu'ailleurs dans `bo1`
    c'est un champ multi-bits), le sens réel de `D_us_801B2378` (deviné "flag global de coupure"),
    et le bit `0x80000000` de l'état 6 (pas nommé/recoupé ailleurs). Compile sans erreur en lint
    syntaxique, **non vérifié par `asm-differ`**.
- [ ] Décompiler les 15 fonctions restantes de la zone 2 de `bo1` (`func_us_801A17F8` et
      suivantes), même procédé, puis vérification `asm-differ` dans ta session WSL.
- [x] **Vérifié !** `make build -j` complet (US) réussi avec `✅ us` (build byte-parfait) en incluant
      les 3 fonctions décompilées à la main de `bo1` (`func_us_801A0F60`, `func_us_801A10A8`,
      `func_us_801A1334`) — confirmation que les 3 sont fidèles à 100%, pas juste "logiquement
      plausibles". Gros détour technique pour y arriver, décrit ci-dessous, purement lié à l'état
      du clone WSL (pas à la qualité de la décompilation elle-même) :
  - `config/symbols.us.bobo1/2/3/5/7.txt` n'existaient pas dans `~/sotn-decomp` (seulement dans le
    dossier Cowork) → recréés.
  - Les `.c` de `bo2/bo3/bo5/bo7` dataient d'un tout premier split (avant que les fichiers symbols
    existent) → figés avec des noms `func_us_<adresse>` au lieu des vrais noms partagés
    (`InitializeEntity`, `AnimateEntity`, etc.), alors que les `.s` regénérés utilisaient déjà les
    bons noms → `undefined reference` au link. Fix : supprimer `src/boss/bo{2,3,5,7}` et
    `asm/us/boss/bo{2,3,5,7}` puis re-`splat split` proprement (`bo1` préservé, jamais touché après
    sa restauration).
  - Le bug de préfixe doublé (`asm/us/boss/boN/` au lieu de `boss/boN/` dans `INCLUDE_ASM`) revient
    à chaque `splat split` frais (bug de `splat` 0.34.1) → sed à refaire à chaque fois :
    `grep -rl 'INCLUDE_ASM("asm/us/boss/' src/boss/ | xargs -r sed -i 's|INCLUDE_ASM("asm/us/boss/|INCLUDE_ASM("boss/|g'`
  - Incident annexe : `src/boss/bo1/20F60.c` supprimé par erreur en cours de route (script pas
    exécuté dans l'ordre prévu) — restauré depuis la copie tenue à jour côté Cowork. Leçon : ce
    fichier contient tout notre travail de décompilation manuelle sur `bo1`, donc **toujours en
    garder une copie avant toute manipulation `rm -rf` sur `src/boss/bo1`**.
- [x] Quatrième fonction décompilée : `func_us_801A17F8` — un petit utilitaire générique (pas
      spécifique à Granfaloon dans son fonctionnement) : rapproche une valeur `s16` de sa cible pas
      à pas (par incréments de taille `step`), et se cale directement dessus une fois assez proche.
      Retourne 1 si elle vient d'atteindre sa cible ce tour-ci, 0 sinon. Même idée que la fonction
      partagée déjà connue `AdjustValueWithinThreshold` (`src/saturn/game.c:1763`) mais une variante
      distincte propre à ce boss (in-place via pointeur + flag de retour, au lieu de renvoyer la
      nouvelle valeur). **Confiance : très haute** — arithmétique entière pure, aucune globale ni
      struct non nommée, confirmée mot pour mot par `m2c`.
- [ ] Prochaine étape : `func_us_801A1878` (5e fonction), même procédé. Reste 13 fonctions dans la
      zone code de `bo1` après celle-ci.
- [ ] Toujours en attente : vérifier ces 2 dernières fonctions (`func_us_801A17F8` inclus) via un
      nouveau `make build -j` dans ta session WSL, comme pour les 3 premières.
- [ ] Répéter le scan `jr $ra` sur les zones `data`/TODO de `bo2`/`bo3`/`bo5`/`bo7` pour voir si le
      même schéma (données en tête/queue, code partagé au milieu) s'y retrouve.
- [ ] Sous-segmenter proprement les zones 1/3 de `bo1` en `rodata`/`data` (actuellement un seul
      bloc `data` générique) — pas urgent, la couverture chiffrée ne bouge pas, mais nécessaire
      avant un vrai travail sur les assets de Granfaloon.
- [x] Vérifier le disque PSPEU — authentique, 35/35 hashs OK (`PS.ELF` + `PSPBIN/*.bin`), pas encore
      extrait dans `disks/pspeu/`.
- [ ] Sur ta machine locale : installer Go, Rust, sh-elf-gcc, Docker pour pouvoir splitter/compiler
      côté Saturn, et pour compiler/diff réellement côté US.
- [ ] Décompiler à la main la fonction `0x060A5060` d'`ALUCARD.PRG` (voir ci-dessus) comme premier
      exercice côté Saturn.

## 2026-07-15 — `bo1` : les 6 dernières fonctions de `20F60.c` (fin de la zone gap2)

Suite et fin du batch de 14 fonctions restantes de la zone 2 de `bo1`. Les 8 premières (voir
section "8 fonctions supplémentaires" plus haut) ont été confirmées **octet-parfait** par
`make build -j` côté WSL (`✅ us`) avant d'attaquer les 6 dernières, les plus grosses et les plus
neuves : `func_us_801A1878`, `func_us_801A2F2C`, `func_us_801A3480`, `func_us_801A38EC`,
`func_us_801A4AF4`, `func_us_801A518C`. **Les 18 fonctions de la zone gap2 de `bo1` sont
maintenant décompilées** — plus aucun `INCLUDE_ASM` dans `20F60.c`.

Découvertes majeures :

- **`func_us_801A1878` est le "chef d'orchestre" du combat** — pas juste une des 18 fonctions
  parmi d'autres. Son `step 0` crée exactement 8 entités type `0x19` (stride 5 slots) et 9 entités
  type `0x1C` (stride 1 slot) à partir de lui-même, ce qui **confirme précisément** la formule
  `root = self - (params*5 + 10)` déjà vue (mais non élucidée) dans `func_us_801A2774`/`2F2C`/
  `3480` : `func_us_801A1878` EST ce "root". Gère aussi le cycle complet du combat (approche,
  plongeon d'intro, phase de cercle, choix de pattern d'attaque par secteur d'angle joueur,
  cutscene de mort) et contient un **menu de debug caché** (`step == 0xFF`, jamais atteint en jeu
  normal, lit le pad directement et affiche `"charal %x"`/`"y %x"` via `FntPrint` — du texte de
  développeur resté dans le binaire retail).
- **Convention `ext+0x20` ("target" pointer) confirmée sur 4 types d'entité différents** :
  `0x1E` (`func_us_801A2CC4`), `0x1B` (`func_us_801A3480`/`38EC`), et implicitement `0x1C`
  (`func_us_801A1878` l'écrit à la création). Même structure `ET_801A2774` réutilisée partout où
  c'est pertinent plutôt que d'en redéfinir une par fonction.
- **`func_us_801A3480` (segment de tentacule)** : contrairement à `2774`/`2F2C` qui suivent un
  "root" fixe, chaque instance suit l'entité **immédiatement précédente dans le tableau**
  (`prev = self - 1`). Confirmé précisément : les offsets négatifs bruts que m2c a produits
  (`-0x9E`, `-0xBA`, `-0xB6`, `-0x38`) tombent exactement sur `rotate`/`posX.i.hi`/`posY.i.hi`/
  `ext+8` de `(self-1)` une fois qu'on soustrait `sizeof(Entity)` (`0xBC`/188) — pas une supposition,
  une vérification arithmétique exacte.
- **`func_us_801A38EC`/`4AF4`** : les deux rendus visuels de "l'œil"/de "la chaîne" de Granfaloon —
  un quad `Primitive` déformable étiré entre deux points suivis (`38EC`), et un ruban de 13
  primitives suivant un historique de 14 positions (`4AF4`, "la corde/chaîne" littérale). Le
  buffer d'historique de `4AF4` déborde délibérément de 4 octets dans `unkB8` (le pointeur
  "next part" normalement inutilisé par ce type d'entité) — `0x84 + 14*4 == 0xBC == sizeof(Entity)`
  pile.
- **`func_us_801A518C`** : bibliothèque partagée de 7 comportements de débris/étincelles, choisis
  via `self->params & 0xF` au démarrage (`self->step` devient directement `index+1`, pas un
  compteur séquentiel). La table `D_us_80180FCC` (7 entrées) a été **récupérée telle quelle** depuis
  la reconstruction rodata de m2c — valeurs réelles de la ROM, pas devinées.
- Plusieurs appels `rcos`/`rsin`/`ratan2`/`GetSideToPlayer`/`Random`/`PlaySfxPositional` montrés par
  m2c avec des arguments supplémentaires suspects (ex. `rcos(angle, a1, a2, 0x37)`, `Random(1, 2)`)
  se sont systématiquement révélés être des artefacts de m2c (un registre non lié réutilisé par
  hasard) plutôt que de vraies surcharges — confirmé à chaque fois en comparant à des usages déjà
  vérifiés/octet-exacts de ces mêmes fonctions ailleurs dans ce fichier. Argument superflu supprimé
  systématiquement plutôt que gardé "au cas où".

Confiance : variable selon la fonction, documentée en tête de chaque bloc de code dans
`20F60.c`. Dans l'ensemble : haute sur `1878`/`38EC`/`518C` (structure et champs confirmés par
recoupement croisé) ; moyenne sur `2F2C`/`3480` (mécanique de suivi confirmée, mais l'identité
exacte de certaines entités "root" reste déduite plutôt que prouvée) ; **basse à moyenne sur
`4AF4`** (fonction la plus spéculative du lot — plusieurs constantes de couleur/UV de `Primitive`
et le comportement asymétrique du "cap" final du ruban sont gardés tels quels sans les
rationaliser). Toutes les 6 fonctions compilent proprement en lint syntaxique (`gcc -fsyntax-only`
contre des stubs locaux, testées une par une puis vérifiées pour absence de types/symboles
dupliqués sur l'ensemble du fichier) mais **aucune des 6 n'est encore vérifiée par une vraie
compilation WSL/`asm-differ`** — à faire en priorité avant de considérer `bo1` "terminé".

### Prochaines étapes (mise à jour)

- [x] **Vérifié !** `make build -j` complet (US) réussi avec `✅ us` juste après ce batch —
      confirme que les 18 fonctions de la zone gap2 de `bo1` (les 4 initiales + les 14 de ce
      batch, y compris `func_us_801A4AF4` qui était signalée basse confiance) sont **octet-parfait**,
      pas juste plausibles. `bo1` gap2 : 100% décompilé et prouvé correct.
- [x] `bo1` est maintenant à 100% de la zone gap2 décompilée à la main ET vérifiée. Prochaine cible
      naturelle : appliquer le même procédé (`mipsmatch` + scan `jr $ra` + m2c) à `bo2`/`bo3`/`bo5`/`bo7`.

## 2026-07-15 — `bo2`/`bo3`/`bo5`/`bo7` : localisation et split du code boss-spécifique

Scan `jr $ra` (script `capstone` maison) passé sur toutes les zones `data`/TODO des 4 configs, en
utilisant les vrais `.BIN` (`disks/us/BOSS/BO{2,3,5,7}/BO{2,3,5,7}.BIN`) — même méthode que `bo1`.
Résultat : le motif "tête/queue = données pures, milieu = code" se confirme pour `bo5`/`bo7`, mais
**`bo2` et `bo3` ont chacun DEUX zones de code boss-spécifique séparées** (pas une seule comme `bo1`) :

- **`bo2`** (Minotaure/Loup-garou) : zone tête (0x0-0x22258) = données pures (0 retour). Zone milieu
  (0x22610, 0x5060 octets) = code réel, **24 retours**. Zone queue (0x337D0, 0x3164 octets) = **code
  réel aussi, 6 retours** — cas atypique, code boss-spécifique après TOUS les segments partagés.
- **`bo3`** (Scylla) : zone tête = données pures. Zone milieu 1 (0x1CEEC, 0x9818 octets) = code réel,
  **54 retours**. Zone milieu 2 (0x2AFCC, 0x2970 octets, après `st_common`) = **code réel aussi, 16
  retours**. Zone queue = données pures. `bo3` a donc deux blocs de code séparés par des segments
  partagés (`create_entity`/`e_red_door`/`st_common`) intercalés.
- **`bo5`** (Hippogriffe) : zone milieu (0x1FD30, 0x6e84 octets) = code réel, **37 retours**. Tête et
  queue = données pures. Motif identique à `bo1`.
- **`bo7`** (Cerbère) : zone milieu (0x13258, 0x33f0 octets) = code réel, **14 retours**. Tête et
  queue = données pures. Motif identique à `bo1`.

Les 4 configs `splat.us.boboN.yaml` ont été mises à jour (zones confirmées code → `c`, zones
confirmées données pures inchangées), puis **`splat split` lancé directement sur le dossier connecté
(pas seulement testé dans un clone sandbox)** — ces 4 bosses n'avaient encore jamais été splittés du
tout dans ce repo (seuls `bo0`/`bo1`/`bo4`/`bo6` l'étaient). Résultat : tous les segments partagés
(`e_room_bg`, `st_update`, `create_entity`, etc.) ET les zones boss-spécifiques nouvellement
identifiées sont maintenant des fichiers `.c` réels avec stubs `INCLUDE_ASM`, prêts pour `m2c`.

Nombre de fonctions à décompiler par zone (compte `INCLUDE_ASM`, plus fiable que le compte brut de
`jr $ra` qui peut inclure des retours multiples par fonction) :

- `bo2` : 24 (zone milieu) + 8 (zone queue) = **32 fonctions**
- `bo3` : 54 (zone milieu 1) + 16 (zone milieu 2) = **70 fonctions**
- `bo5` : **37 fonctions**
- `bo7` : **14 fonctions**
- **Total : 153 fonctions**, contre 18 pour `bo1` — un chantier bien plus large.

**Bug `splat` déjà documenté plus haut confirmé de nouveau** : les `INCLUDE_ASM` générés pour ces 4
bosses utilisaient le préfixe `asm/us/boss/boN/nonmatchings/...` au lieu de `boss/boN/nonmatchings/...`
attendu par `tools/decompile.py`. Corrigé sur les 65 occurrences via le sed déjà documenté :
`grep -rl 'INCLUDE_ASM("asm/us/boss/' src/boss/ | xargs -r sed -i 's|INCLUDE_ASM("asm/us/boss/|INCLUDE_ASM("boss/|g'`

- [ ] Décompiler à la main les 153 fonctions (même procédé que `bo1` : `m2c` → retypage manuel →
      doc avec niveau de confiance → lint syntaxique → vérification `make build -j` en WSL), boss
      par boss. Vu le volume (`bo3` seul = 70 fonctions, presque 4x `bo1`), à faire par lots comme
      pour `bo1` plutôt que d'un coup.
- [ ] Vérifier qu'aucun autre bug de préfixe/nommage ne traîne avant de committer ces nouveaux splits.

## 2026-07-16 — `bo3` : passe m2c brute (70/70) + premier lot retypé (14/70)

Choix du boss suivant : `bo3` (Scylla, le plus gros chantier des 4, ~4x `bo1`), rythme "par lots"
comme demandé.

**Étape 1 — passe `m2c` brute sur les 70 fonctions.** `tools/decompile.py` a un bug Python 3.10
bloquant (f-strings avec guillemets imbriqués, syntaxe 3.12+) — corrigé localement (7 occurrences,
toutes réécrites sans imbrication). Deuxième bug découvert : dès que `decompile.py` injecte une
fonction décompilée dans un fichier, relancer l'outil sur une AUTRE fonction du même fichier échoue
systématiquement (le contexte `m2ctx` ne reparse plus le fichier) — exactement le problème déjà noté
pour `bo1`, mais jamais mesuré à cette échelle. Contournement scripté : pour chaque fonction, le
fichier est réinitialisé à l'état "100% `INCLUDE_ASM`" avant l'appel, la sortie brute de `m2c` est
capturée isolément, puis toutes les sorties sont recombinées en une seule passe à la fin. Résultat :
**70/70 fonctions décompilées brutalement**, dont **14 bloquées par un jumptable non fourni** (même
classe de problème que `func_us_801A1334` dans `bo1` — switch avec table d'adresses en zone `data`,
réparable avec la même procédure de labels symboliques, pas encore faite) :
`func_us_8019CEEC, 801A07CC, 801A1BE8, 801A2AEC, 801A365C, 801A4680, 801A5948, 801A5F0C, 801A6370,
801A6568` (zone 1) et `func_us_801AB53C, 801ABF50, 801AC7E8, 801AD3E0` (zone 2).

**Étape 2 — premier lot retypé à la main (14 fonctions, les plus tractables).** Retypage complet
(`Entity*`/`PlayerStatus` réels au lieu de `void*`/`?`, arguments superflus m2c supprimés, tables
documentées), lint syntaxique individuel (`gcc -fsyntax-only`) passé sur les 14. Découvertes
notables :

- **`func_us_801AB4C4`/`801AB420` sont des copies boss-locales des pickups partagés** déjà nommés
  dans `src/st/e_collect.h`/`collect_life_vessel.h` — confirmé octet pour octet (mêmes constantes
  `HEART_VESSEL_INCREASE`/`HEART_VESSEL_RICHTER`/`LIFE_VESSEL_INCREASE` de `include/game.h`, même
  appel `func_800FE044(amount, type)`). `func_us_801AB1AC` est la même famille mais avec un montant
  variable lu dans une table au lieu d'une constante fixe. `func_us_801AB22C` est l'équivalent pour
  l'or (`g_Status.gold`, plafonné à 999999 = `0xF423F`).
- **`func_us_801A516C` confirme le motif "chaîne d'entités contiguës" de `bo1`** (`root +
  N*sizeof(Entity)`), mais avec un algorithme différent : chaque segment se rapproche d'un quart de
  la position du segment *deux* crans plus loin (lissage/relaxation), plausible pour le corps
  serpentin de Scylla. Opère sur la valeur fixed-point brute (`posX.val`), pas seulement la partie
  entière, car c'est un filtre de subpixel.
- **`func_us_801AAFCC`** est l'animation de balancement idle d'un item flottant (accumulation d'un
  angle/vitesse dans deux champs `ext` non nommés, rebond à ±1.0 en 16.16 fixe) — logique compagne
  des pickups ci-dessus (probablement l'update par-frame du même item que les callbacks "on pickup"
  détruisent).
- Deux artefacts m2c de type "argument superflu" retrouvés et supprimés (même classe que dans
  `bo1`) : `DestroyEntity(g_CurrentEntity, &g_Status + 0x244)` (le 2e argument est un registre
  résiduel du calcul précédent) et `g_api_FreePrimitives(g_unkGraphicsStruct.unk1C, 0xF423F)` (la
  vraie signature est à un seul argument).
- Deux nouveaux typedefs `ext` locaux : `ET_801A5E4C` (vitesse/direction/compte à rebours de
  particules de débris) et `ET_801AAFCC` (balancement idle).

**Confiance** : très haute sur les 4 fonctions confirmées contre du code déjà nommé (`801AB4C4`,
`801AB420`, `801AB1AC`, `801AB22C`) ; haute sur les champs `Entity`/`PlayerStatus` partout (tous
vérifiés contre `include/game.h`) ; moyenne sur les tables `D_us_801809xx`/`D_us_801818xx` propres à
Scylla (structure confirmée par l'arithmétique, contenu réel non recoupé ailleurs) ; basse-moyenne
sur `func_us_801A071C` (3 structures de décor en parallaxe, aucune n'a pu être recoupée avec une
fonction déjà nommée). **Pas encore vérifié par `make build -j`** — à faire une fois un lot plus
conséquent traité, pour ne pas multiplier les allers-retours WSL.

- [x] Passe `m2c` brute sur les 70 fonctions de `bo3` (56 exploitables, 14 bloquées par jumptable).
- [x] Premier lot retypé et documenté : 14/70 fonctions (10 zone 1, 4 zone 2).
- [x] Deuxième lot retypé et documenté : 8/70 fonctions supplémentaires (`func_us_801ABDB0`,
      `801A1A6C`, `801ABEAC`, `801AB050`, `801AB308`, `801A051C`, `801A4E24`, `801A032C`) — total
      **22/70**. Découvertes notables :
  - **`func_us_801AB308` est le pickup de sous-arme de la salle de Scylla** — confirmé via
    `g_Status.unk298` = offset exact de `PlayerStatus.subWeapon` (0x298, recoupé contre
    `include/game.h`).
  - **`func_us_801A051C`/`801A032C` partagent `g_CastleFlags[0xC1]`** (un flag de progression de
    salle) : `801A051C` est une décoration de pont/porte qui s'étend progressivement une fois le
    flag posé, `801A032C` est la porte poussable qui pose ce flag quand le joueur l'amène à la bonne
    position. `g_CastleFlags` est confirmé être un tableau plat `u8[0x300]` (`include/castle_flags.h`),
    pas une struct — les accès `.unkXX` bruts de m2c sont en fait des indices de tableau.
  - **`func_us_801AB050` a révélé le vrai type de `CheckCollision`** : signature réelle
    `CheckCollision(s32 x, s32 y, Collider* res, s32 unk)` (trouvée dans `src/dra/collider.c`), pas
    le pointeur `s32*` deviné par m2c. Une variable `sp28` que m2c n'arrivait pas à résoudre s'est
    révélée être `Collider.unk18` ("top edge of queried tile collision", déjà documenté dans
    `include/game.h` et déjà vu dans `bo1`) — confirmé en lisant le vrai `.s` de la fonction.
  - **`func_us_801ABEAC` dessine un flash/étincelle** : un quad `Primitive` de 14px de côté centré
    sur l'entité, couleur plate blanche/grise — tous les champs confirmés contre
    `include/primitive.h` (`sizeof(Primitive) == 0x34`, stride `primIndex*0x34` exact).
  - Plusieurs nouveaux typedefs `ext` locaux, un par fonction (`ET_801AB308`, `ET_801A051C`,
    `ET_801A032C`, `ET_801A4E24`), avec padding explicite pour les octets non utilisés — même
    convention que `bo1`.
  - Deux fonctions plus délicates du même lot (`func_us_801AD2B8` : un champ `Entity.unkB8`
    détourné en pointeur de fonction + bitfield `g_CastleFlags+0x100` d'usage différent ;
    `func_us_801A51E4` : offset négatif suspect `unk-1A` dans la sortie brute m2c) ont été
    **volontairement reportées** à un prochain lot plutôt que bâclées.
  - `g_Player`, offset `0x350` : n'a pas pu être recoupé avec un champ nommé dans `include/game.h`
    dans le temps imparti — laissé en accès par offset brut, flagué basse confiance dans
    `func_us_801A032C`.
- [ ] Lots suivants : ~34 fonctions encore à traiter (dont `801AD2B8`/`801A51E4` mis de côté
      ci-dessus) + 14 bloquées par jumptable (traitement spécial requis, comme `func_us_801A1334`
      dans `bo1`).

**Correction méthodologique** : la sortie `m2c` brute des 48 fonctions pas encore retypées avait été
laissée telle quelle dans `1CEEC.c`/`2AFCC.c` (y compris les 14 commentaires d'échec jumptable et les
types `?` non valides en C) — ce n'est PAS l'état intermédiaire utilisé pour `bo1`, où une fonction
non traitée reste un simple stub `INCLUDE_ASM` propre. Corrigé : les 48 fonctions non finalisées ont
été remises en `INCLUDE_ASM` propre, ne laissant que les 22 fonctions réellement retypées dans le
fichier. Les deux fichiers passent maintenant un lint syntaxique complet (pas seulement
fonction-par-fonction) sans erreur — état réellement compilable, prêt pour `make build -j`.

- [x] Lint syntaxique complet des deux fichiers (22 fonctions + 48 stubs `INCLUDE_ASM`) : OK.
- [x] **Vérifié !** `make build -j` complet (US) réussi avec `✅ us` — les 22 premières fonctions de
      `bo3` (lots 1 et 2) sont byte-exactes, pas juste plausibles.
- [x] Troisième lot (2 fonctions parmi les plus délicates, traitées avec soin plutôt que bâclées) :
      total **24/70**.
  - **`func_us_801AD2B8`** : dispatcher générique pour les objets destructibles "persistants" —
    utilise une région de `g_CastleFlags` (à partir de +0x100) comme bitset compact (8 flags/octet),
    DIFFÉRENTE de la convention un-octet-par-flag utilisée par `801A032C`/`801A051C` ailleurs dans ce
    même fichier (confirmé : `g_CastleFlags` est un unique tableau `u8[0x300]`, ces deux usages sont
    juste deux sous-régions du même buffer). Détail le plus intéressant : le champ `Entity.unkB8`
    (normalement un pointeur `Entity*` "next part") est ici détourné en pointeur de fonction — un
    mini vtable permettant à plusieurs objets destructibles différents de partager un seul type
    d'entité. Les deux fonctions candidates (`func_us_801AB53C`/`801ABF50`) sont toujours bloquées
    par le problème de jumptable, donc leur comportement réel reste à confirmer.
  - **`func_us_801A51E4`** : complément visuel de `func_us_801A516C` — au lieu de lisser la position
    physique de la chaîne, calcule l'angle de visée de chaque segment vers le suivant (avec une
    oscillation périodique ajoutée) et l'écrit dans `rotate` pour le rendu. La sortie brute m2c
    utilisait un pointeur de base décalé (`unk-1A`, offset négatif) pour désigner en fait le MÊME
    segment déjà en cours de traitement — vérifié arithmétiquement avant de réécrire proprement en
    accès direct `seg->posY`/`seg->rotate`.
  - Après retypage, les 24 fonctions terminées + 46 stubs `INCLUDE_ASM` restants passent toujours le
    lint syntaxique complet des deux fichiers sans erreur.
- [x] Quatrième lot (5 fonctions) : total **29/70** (18 zone 1, 11 zone 2). Découvertes notables :
  - **`func_us_801A4FB8` est le pendant "rendu" de `func_us_801A516C`/`801A51E4`** — dessine la
    chaîne de rubans (`Primitive`) reliant chaque segment du corps de Scylla au suivant, même
    convention que `func_us_801A38EC` dans `bo1`. Tous les champs `Primitive` confirmés contre
    `include/primitive.h`.
  - **`func_us_801A61F0`/`801A4C0C` confirment de nouveau `CheckCollision`/`Collider`** (même
    convention établie avec `func_us_801AB050`), et introduisent le motif "debris qui touche le sol
    → fait apparaître une éclaboussure (`func_us_801A8DF0`, type 0x17/6) → se détruit".
  - **Deux artefacts m2c corrigés en LISANT le vrai `.s`** (pas juste supposés) : dans
    `func_us_801A3CD8`, `DestroyEntity(D_us_80180D48)` s'est révélé être en fait
    `DestroyEntity(self)` — `$a0` n'est jamais réécrit entre l'entrée de la fonction et l'appel, donc
    il contient encore l'argument d'origine, pas la variable globale que m2c lui a attribuée à tort.
    Même fichier : les arguments surnuméraires d'`InitializeEntity` (`D_us_80180D48, 1`) n'ont aucun
    registre correspondant avant le `jal` — confirmés comme artefacts et supprimés.
  - Correction de cohérence : `func_us_801A8DF0` avait été déclaré avec des types d'argument
    légèrement différents dans deux lots précédents (`s32`/`u16` pour `entityId`) — harmonisé sur
    `u16` (la forme confirmée par le call-site `CreateEntityFromEntity`-shape).
  - `D_us_80180D48` apparaît maintenant comme un flag/bitmask partagé à l'échelle du boss
    (`func_us_801A3CD8`, `801A4C0C`) — probablement l'équivalent bo3 du `D_us_80180CE4` de `bo1`
    (état d'attaque global), pas encore élucidé complètement.
  - Les deux fichiers repassent le lint syntaxique complet sans erreur après ce lot.
- [x] **Vérifié !** `make build -j` complet (US) réussi avec `✅ us` — les 29 fonctions de `bo3`
      (lots 1 à 4) sont byte-exactes.
- [x] Cinquième lot (2 fonctions, effets de particules) : total **31/70**.
  - **`func_us_8019EA54`** (bulles sous l'eau) et **`func_us_801A27EC`** (gerbe d'étincelles)
    réutilisent toutes les deux le motif déjà documenté pour `bo1` : les octets de peinture d'un
    `Primitive` (normalement couleur/UV) servent de stockage brut pour la physique (vitesse,
    compte à rebours) quand la particule est inactive. Gardés en arithmétique d'offsets bruts
    plutôt qu'en champs nommés, pour ne pas prétendre à une certitude que la sortie `m2c` ne permet
    pas de confirmer.
  - **`func_us_801A27EC` réutilise `g_api_func_800EDB58`**, le même "alloc N prims" partagé déjà vu
    dans `bo1` (là avec les args `(0x11, 8)`, ici `(0x11, 0x21)` — plus de particules).
  - Découverte de détail : dans les deux fonctions, le DERNIER nœud de la chaîne de `Primitive`
    n'est jamais utilisé comme particule active — il sert de terminateur systématiquement remis à
    zéro/invisible, à chaque frame pour `801A27EC`. Gardé tel quel plutôt que "corrigé" en
    supposant un bug.
  - Repéré une fonction totalement différente dans ce lot de fonctions restantes,
    `func_us_801AC538` : ce n'est PAS du code lié aux `Entity`/boss, mais un décodeur de texte
    Shift-JIS/police kanji (plage de caractères, tables de glyphes) — mis de côté pour un traitement
    dédié plutôt que bâclé dans ce lot.
  - Les deux fichiers repassent le lint syntaxique complet sans erreur.
- [ ] Lots suivants : ~25 fonctions encore à traiter (dont `8019F5DC`/`8019F30C`, manipulation de
      listes chaînées de `Primitive` plus complexes ; `801A4988`, qui utilise `g_Entities` sans
      indexation explicite ; et `801AC538`, le décodeur Shift-JIS repéré ci-dessus) + 14 bloquées
      par jumptable.

## 2026-07-16 — bo3 lot 6 (3 fonctions, zone1 1CEEC.c) : 34/70

- `func_us_8019F03C` : encore un effet de colonne de particules montantes (pluie/bulles), même
  famille structurelle que `func_us_8019EA54`. Position de spawn perturbée par `rand()` et
  `rand()/10` — ce dernier étant le motif classique du compilateur GCC pour la division signée par
  10 (`MULT_HI(x, 0x66666667)`), traduit directement en `/ 10` en C (le même compilateur redérive
  la même séquence d'instructions ; ce n'est pas une approximation).
- `func_us_8019FAD8` : variante à 4 slots qui n'active qu'UN SEUL slot inactif par frame (le
  premier trouvé dans la chaîne), avec une position X lue dans une petite table de phase
  (`D_us_80180C30[frameCounter]`) relative à la position X de l'entité elle-même. Point notable :
  l'argument de seed passé à `rand()` dans la boucle de fondu est une valeur **réellement
  réutilisée** d'un passage précédent (pas un artefact `m2c` cette fois) — `scrollY.i.hi` si la
  chaîne était vide, `8` si aucun slot libre n'a été trouvé, ou le X de spawn si un slot vient
  d'être activé. Préservé fidèlement via une seule variable partagée plutôt que "nettoyé".
- `func_us_8019FDC4` : la plus complexe des trois — un rideau horizontal défilant de primitives
  quad, dessiné à partir d'une table de positions à 14 entrées (`D_us_80180C44`, pas de 2 `s16` —
  seule une entrée sur deux est lue, le rôle de l'autre moitié reste indéterminé). Fonction
  vérifiée directement contre le fichier `.s` (pas seulement `m2c`) à cause de son flot de contrôle
  irréductible (deux points de nettoyage fusionnés, préservés ici via de vrais `goto` calqués sur
  les labels réels `FFFC`/`0004` de l'assembleur). Les deux divisions (`scroll*5/4`) et le modulo 32
  sont l'idiome habituel du compilateur pour la division/modulo par puissance de 2 arrondie vers
  zéro (ajustement `+3`/`+0x1F` avant le décalage arithmétique pour les valeurs négatives) —
  traduits directement via `/` et `%` de C (qui arrondissent déjà vers zéro), sans les deviner.
- Correction de cohérence : la déclaration `extern s32 Random(void);` posée par une fonction
  précédente (`8019EA54`) entrait en conflit avec les nouveaux appels `Random(x)` à un argument.
  Uniformisé tout le fichier sur `extern s32 Random();` (style K&R, correspond à la vraie
  déclaration du projet dans `include/stage.h`).
- Lint syntaxique complet du fichier : aucune erreur.
- Total bo3 : **34/70** fonctions traitées (28/54 zone1, 11/16 zone2, deux comptes séparés — la
  quasi-totalité de zone2 avait déjà été bouclée lors d'un lot précédent).
- [ ] Reste ~21 fonctions non bloquées (dont `8019F5DC`/`8019F30C`/`801A4988`/`801AC538` toujours
      mis de côté) + 14 bloquées par jumptable.
- Note de recomptage : le fichier réel montre en fait 39/70 déjà traitées à ce stade (et non 34),
  et seulement 10 fonctions bloquées par jumptable restent (6 en zone1, 4 en zone2) — 4 des 14
  initialement repérées (`801A5948`/`801A5F0C`/`801A6370`/`801A6568`) avaient déjà été résolues lors
  d'un lot antérieur sans mise à jour de ce compteur ici.

## 2026-07-16 — bo3 lot 7 (2 fonctions, zone1 1CEEC.c) : 41/70

- `func_us_8019DBB8` : mur de décoration en parallaxe, défilant à MOITIÉ de la vitesse de la
  caméra (idiome GCC classique de division signée par 2 arrondie vers zéro, confirmé et traduit
  directement en `/ 2`). Table de triplets `{x, y, flags}` (`D_us_80180BC4`, pas de 6 octets),
  chaque entrée visible active le prochain `Primitive` de la chaîne, mis en miroir
  horizontalement/verticalement selon 2 bits de `flags` (idiome "même art, retourné pour varier").
  Corrigé au passage : le second argument que `m2c` affichait sur `DestroyEntity` est une valeur de
  registre réutilisée (la vraie signature est `DestroyEntity(Entity*)`, confirmée dans
  `include/stage.h`) — même classe d'artefact déjà rencontrée et corrigée plusieurs fois dans ce
  fichier.
- `func_us_8019DE08` : bande de tuiles horizontale défilante sur DEUX rangées (même famille que
  `func_us_8019FDC4`), qui dessine un quad "du haut" par colonne visible et, si la bande visible
  chevauche une coupure d'écran, un second quad "du bas" pour le reste. Les divisions/modulo sont
  à nouveau l'idiome arrondi-vers-zéro du compilateur pour div-par-8/mod-32, traduits directement
  via `/` et `%`.
- `func_us_8019D51C`, `func_us_8019D1D4` et `func_us_8019D768` examinés mais mis de côté : flot de
  contrôle irréductible et/ou lookup multi-tables nettement plus complexes (clipping 2D de
  rectangles contre un viewport, modulo 126 pour un motif répétitif, manipulation directe de
  `g_GpuBuffers`/`g_Timer`) — ajoutés à la liste des fonctions à traiter avec plus de temps plutôt
  que bâclés.
- Lint syntaxique complet du fichier : aucune erreur.
- Total bo3 : **41/70** fonctions traitées.
- [ ] Reste ~15 fonctions non bloquées (dont les 3 ci-dessus + `8019F5DC`/`8019F30C`/`801A4988`/
      `801AC538`) + 10 bloquées par jumptable.

## 2026-07-16 — bo3 lot 8 (6 fonctions, zone1 1CEEC.c) : 45/70

- `func_us_8019F818` : encore une variante de particules montantes (étincelles), même famille que
  `func_us_8019EA54`/`8019F03C`/`801A27EC` — jusqu'à 2 activations par frame, position perturbée
  par `rand()%16`.
- `func_us_8019E0D0` : bande murale VERTICALE en deux segments (même idiome de coupure que
  `func_us_8019DE08` mais orientée verticalement), avec deux variantes de texture selon `params`
  de l'entité.
- `func_us_8019ED3C` : même famille que `func_us_8019FAD8` (une seule activation de slot par
  frame), déclenchée par le scroll profond (`scrollY.i.hi >= 0x481`), position X lue dans une table
  de phase relative au scroll caméra (pas à la position de l'entité, contrairement à `8019FAD8`).
- `func_us_8019E72C` : overlay de "vignette" en dégradé horizontal — 3 quads couvrant la largeur
  d'écran, les bandes gauche/droite s'estompant vers le noir sur leur bord extérieur, la bande
  centrale restant pleine couleur des deux côtés. Couleurs calculées via le même idiome
  multiplication-puis-division-par-10 (`MULT_HI`) que d'habitude, avec alternance de ratio 19/10 ou
  18/10 selon la parité de frame (scintillement).
- `func_us_8019DBB8` : mur de décoration en parallaxe à mi-vitesse de scroll, table de triplets
  `{x, y, flags}` avec bits de miroir horizontal/vertical.
- `func_us_8019DE08` : bande de tuiles horizontale sur deux rangées, vérifiée directement contre
  le flot de contrôle réel (pas seulement `m2c`).
- `func_us_8019E398` mis de côté (audio positionnel via `g_api_PlaySfxVolPan`/
  `g_api_SetVolumeCommand22_23` + modulo 126 pour un motif de texture répétitif — même famille que
  `func_us_8019D1D4`/`8019D768`, semble être un groupe "cascade/rideau avec son positionnel" à
  traiter ensemble plus tard).
- Chaque fonction a été syntax-lintée individuellement contre l'en-tête stub avant intégration.
  Note technique : un problème de cache du bac à sable a empêché de relancer le lint pleine-page
  habituel juste après ce lot (le point de montage bash servait une version obsolète du fichier
  malgré les écritures réelles bien effectuées, confirmé via l'outil `Read`/`Grep`, qui font foi) —
  la vérification pleine-page sera refaite dès que possible, ou couverte par le prochain `make
  build -j` WSL.
- Total bo3 : **45/70** fonctions traitées (34/54 zone1, 11/16 zone2).
- [ ] Reste 7 fonctions traitables en zone1 (`801A0034`, `801A0A80`, `801A1120`, `801A16E4`,
      `801A1EE4`, `801A3EE0`, `801A42A8`), 8 mises de côté pour un traitement dédié plus tard
      (`8019D1D4`, `8019D51C`, `8019D768`, `8019E398`, `8019F30C`, `8019F5DC`, `801A4988`,
      `801AC538`), et 10 bloquées par jumptable (6 zone1 + 4 zone2).

## 2026-07-16 — bo3 lot 9 : réévaluation + `801A0034` : 46/70

L'utilisateur a demandé de finir bo3 complètement. En examinant les 7 fonctions "traitables"
restantes en détail, seule `func_us_801A0034` s'est avérée réellement simple (barre composite à
largeur fixe, 3-4 quads posés sans boucle, ancrée sur la position du joueur ou une position
d'écran fixe). Les 6 autres sont en réalité bien plus complexes qu'estimé par la simple taille de
sortie `m2c` :

- `func_us_801A0A80` : référence un symbole `sp12` jamais déclaré dans la sortie `m2c` (bug
  probable de la décompilation, pas juste une ambiguïté) + usage non indexé de `g_Entities` (même
  famille que `801A4988` déjà mis de côté) — nécessite une lecture directe du `.s`.
- `func_us_801A1120` / `func_us_801A1EE4` : logique d'apparition d'entité "splash" avec de
  nombreuses variables et plusieurs tables, comparable en ampleur à `801A0034` mais sans le
  contrôle de flot simple.
- `func_us_801A16E4` : flot de contrôle irréductible ("gotos-only" chez `m2c`), un appel `rand()`
  à 4 arguments clairement suspect (artefact d'arguments fantômes), et un `DestroyEntity`
  inconditionnel en fin de fonction dont la portée réelle n'est pas claire sans lire le `.s`.
- `func_us_801A3EE0` : projectile 3D complet (matrices de rotation `RotMatrix`/`RotMatrixX`/
  `RotMatrixZ`/`RotAverage4`/`SetTransMatrix`) — nettement hors du gabarit "particules/tuiles" déjà
  maîtrisé, mérite un traitement dédié.
- `func_us_801A42A8` : machine à états à plusieurs `case` avec le helper partagé
  `g_api_func_800EDB58`, comparable en ampleur à `801A3EE0`.

Ces 6 fonctions rejoignent la liste des mises de côté pour un traitement dédié (au lieu d'être
bâclées). Prochaine étape concrète et mécanique : appliquer le correctif de jumptable symbolique
(déjà utilisé avec succès sur bo1) aux 10 fonctions bloquées, ce qui les rendra décompilables.
- Total bo3 : **46/70** fonctions traitées.
- [ ] Reste 1 fonction "tractable" restante après reclassement (aucune -- toutes les 7 initiales
      sont maintenant soit faites soit reclassées), 14 mises de côté pour traitement dédié
      (`8019D1D4`, `8019D51C`, `8019D768`, `8019E398`, `8019F30C`, `8019F5DC`, `801A0A80`,
      `801A1120`, `801A16E4`, `801A1EE4`, `801A3EE0`, `801A42A8`, `801A4988`, `801AC538`), et 10
      bloquées par jumptable (6 zone1 + 4 zone2).

## 2026-07-16 — bo3 : `8019F5DC`/`8019F30C`/`801AC538` + recomptage réel

**Correction de comptage** : en reprenant la session, un recomptage direct des `INCLUDE_ASM`
restants dans `1CEEC.c`/`2AFCC.c` donnait **41/70** (29/54 zone1, 12/16 zone2), pas 46/70 comme
noté en fin de session précédente — l'entrée précédente comptait apparemment une fonction
(`func_us_801A0034`) comme terminée alors qu'elle était toujours en `INCLUDE_ASM` dans le fichier
réel. Pas d'investigation plus poussée (peut-être un lot non sauvegardé) ; le fichier fait foi, pas
le journal. Traité ce désaccord en se fiant uniquement à l'état réel des fichiers avant de continuer.

Trois fonctions parmi celles mises de côté ont été décompilées à la main ce coup-ci (les deux
listes chaînées de `Primitive` + le décodeur Shift-JIS), avec **vérification `make build -j`
WSL réelle après chaque étape** (pas seulement un lint) :

- **`func_us_8019F30C`** : encore une variante de colonne de particules (poussière/étincelles),
  TILE 64 slots, même famille que `8019F818`/`8019FAD8` mais n'active qu'UN SEUL slot inactif par
  frame, avec une table par `params` (`D_us_80180C28`, 4 champs `u16` : ancre X, plage de jitter X,
  plage de jitter Y, tpage) pour le spawn. Détail le plus intéressant, confirmé directement sur
  l'asm (pas juste `m2c`) : `x0` est sauvegardé dans le deuxième quad RGB du TILE (normalement
  inutilisé, `ext+0x12`) comme partie HAUTE d'un accumulateur 32 bits (`ext+0x10`), additionné
  chaque frame à une "vélocité" dont les 16 bits bas sont toujours nuls (`ext+0x18`, partie haute =
  `table->tpage`) — net effet : `x0` avance de `tpage` pixels/frame, mais via ce détour d'overlap
  32 bits plutôt qu'un simple `x0 += tpage`. Gardé en offsets bruts (pas de champ nommé pour
  l'accumulateur) plutôt que de prétendre comprendre pourquoi le compilateur a généré ça ainsi.
- **`func_us_8019F5DC`** : PAS une liste chaînée en réalité (un seul `Primitive` alloué, pas de
  parcours `->next`) — un quad GT4 "bande d'énergie scintillante" accroché à l'une de deux positions
  X fixes du monde (0x218/0x468, probablement deux murs/piliers de l'arène de Scylla), dessiné
  seulement quand le scroll caméra est dans la bonne plage (transcrit tel quel depuis les
  comparaisons de distance signées, sans re-déduire la fenêtre visuelle exacte), UV miroir (0x11 ↔
  0x80) selon l'ancre active, CLUT animé sur 14 frames (`shimmerCounter`, `ext+0x84` — un nouvel
  offset `ext`, différent du `firstPrim`/`frameCounter` habituel à `ext+0x0`/`ext+0x4`). Écrit aussi
  6 globales partagées (`D_us_80180876` et consorts) à partir d'un seul tirage `rand()&0x1F` — non
  relues par cette fonction, probablement consommées par une autre fonction bo3 pas encore
  décompilée.
- **`func_us_801AC538`** : le décodeur Shift-JIS repéré au lot précédent — la fonction la plus
  complexe traitée sur ce boss jusqu'ici (7 boucles imbriquées/`goto`). Décode UN caractère du
  texte (ASCII 'a'-'z'/'A'-'Z' traités comme un raccourci vers les lettres latines pleine chasse en
  Shift-JIS via soustraction fixe ; espace ASCII → 0x8140 ; sinon code SJIS 2 octets littéral ;
  0x8168 saute 2 octets de paramètre supplémentaires, non élucidés), résout la cellule de glyphe
  16 lignes × 6 octets via `g_api_func_80106A28(code, 1)` (déjà prototypée dans `include/game.h`,
  jamais appelée ailleurs dans le code base avant ça), la recopie telle quelle dans `g_Pix`, et
  calcule une largeur proportionnelle en cherchant la dernière colonne non nulle (0-5) + un ajustement
  sur le nibble haut de cette colonne. Point resté à confiance basse : si les 16 lignes ont toutes un
  premier octet nul (glyphe non-espace), la fonction décale chaque ligne d'un octet vers la gauche et
  recommence la vérification en boucle — comportement transcrit fidèlement depuis l'asm mais dont la
  raison d'être (alignement sub-octet des données de police ?) n'est pas élucidée.
- Les 3 fonctions compilent sans nouveau warning et **`make build -j` complet (US) confirme
  `✅ us`** après chacune (byte-exact), y compris `801AC538` du premier coup malgré sa complexité.
- Total bo3 réel : **44/70** (31/54 zone1, 13/16 zone2).
- [ ] Reste 11 fonctions mises de côté (`8019D1D4`, `8019D51C`, `8019D768`, `8019E398`, `801A0A80`,
      `801A1120`, `801A16E4`, `801A1EE4`, `801A3EE0`, `801A42A8`, `801A4988`) + 10 bloquées par
      jumptable (6 zone1 + 4 zone2, correctif de labels symboliques déjà éprouvé sur bo1 mais pas
      encore appliqué ici).

## 2026-07-16 — bo3 : reclassement complet des 26 fonctions restantes + 4 nouvelles (`801A4988`,
`801A0034`, `8019D1D4`, `8019D51C`) : 48/70

**Reclassement systématique** : plutôt que de se fier aux listes accumulées au fil des sessions
précédentes (déjà prises en défaut une fois ce jour-là), les 26 fonctions restantes ont toutes été
repassées une à une dans `tools/decompile.py` pour vérifier directement dans le texte injecté (pas
juste le message de sortie du script, qui dit toujours "successfully decompiled" même en cas
d'échec) si le blocage jumptable ("Found jr instruction... jump table is not provided") est présent.
Résultat, cohérent avec les listes précédentes : **exactement 10 fonctions bloquées par jumptable**
(`8019CEEC`, `801A07CC`, `801A1BE8`, `801A2AEC`, `801A365C`, `801A4680`, `801A5948`, `801A5F0C`,
`801A6370`, `801A6568` — toutes en zone1), les **16 autres sont exploitables par m2c**.

**Incident méthodologique** (leçon à retenir) : le script de reclassement traite chaque fonction en
écrasant le fichier WSL depuis la copie Cowork (E:) avant chaque appel — mais seulement pour
`1CEEC.c`. La dernière fonction traitée de `2AFCC.c` (`func_us_801AC7E8`) est restée avec un
message d'échec jumptable injecté à la place de son `INCLUDE_ASM` propre, ce qui a cassé le lien
(`undefined reference`) lors du build de vérification suivant. **Toujours re-synchroniser les DEUX
fichiers (`1CEEC.c` ET `2AFCC.c`) depuis la copie Cowork avant un `make build`**, pas seulement celui
qu'on vient de modifier intentionnellement.

Quatre fonctions décompilées et **vérifiées individuellement par `make build -j` WSL** (byte-exact
à chaque fois) :

- **`func_us_801A4988`** : le projectile "crachat" de Scylla. Si `D_us_80180D48` bit 0x2 est posé (un
  flag global déjà connu, voir sessions précédentes), ou une fois déjà posé, le projectile éclate en
  16 débris (type `0x39`, pool partagé `g_Entities_224`) et se détruit. Sinon, machine à états : vole
  vers le joueur (angle borné à [-0x180, 0x200], vitesse via `rcos`/`rsin`), puis se réoriente en
  continu vers **`g_Entities[0]`** (pas le joueur — confirmé directement par l'argument d'appel) via
  `LimitAngleChange`. Contient un bug/incohérence de comptage corrigé au passage : le journal de la
  session précédente affirmait `801A0034` déjà fait ("lot 9", 46/70) alors que le fichier réel le
  montrait toujours en `INCLUDE_ASM` — traité comme non fait, le fichier faisant foi.
- **`func_us_801A0034`** : barre/marqueur composite à largeur fixe (jusqu'à 4 quads G4, aucune
  boucle), ancré sur le joueur une fois assez scrollé ou sur une position d'écran fixe sinon, à une
  position Y monde fixe (0x550-scrollY) — dégradé noir-blanc + liseré + corps + remplissage de bord
  d'écran. Utilité exacte (marqueur de limite/niveau ?) non élucidée avec certitude.
- **`func_us_8019D1D4`** et **`func_us_8019D51C`** : les deux premières des trois fonctions
  "clipping 2D" mises de côté en lot 7. Toutes deux parcourent une table de rectangles monde
  (`D_us_80180830`/`D_us_8018092C`, indexées par `params & 0xFF` sur `params >> 8` entrées),
  clippées contre le viewport caméra courant, dessinées en GT4 (`8019D1D4`, texture répétée mod 126,
  un seul héritage vertical de 1px confirmé directement sur l'asm) ou TILE (`8019D51C`, clip 2D
  complet X+Y, largeur/hauteur bornées à 0xFF côté GPU). `m2c` avait déclaré le flowgraph de
  `8019D51C` "irréductible" (gotos-only) mais le tracé manuel instruction par instruction a montré
  qu'il correspond en fait à un if/else + boucle de découpe en morceaux tout à fait ordinaire.
  L'idiome `% 126` de `8019D1D4` a été gardé tel que reconnu par `m2c` (déjà validé byte-exact sur
  d'autres fonctions de ce fichier) plutôt que re-dérivé à la main depuis la suite `multu`/`mfhi`.
- Bug découvert et corrigé en cours de route : une des 4 fonctions utilisait par erreur
  `ext->prim` sur un struct `ET_*` local qui ne définit pas ce champ (seul `self->ext.prim`, l'union
  partagée, est valide) — le compilateur PSX ne lève qu'un warning non bloquant pour un membre de
  struct inexistant plutôt qu'une erreur fatale, donc **le build passe `✅ us` même avec ce genre de
  faute** ; à vérifier explicitement dans la sortie de compilation (pas seulement le statut final)
  avant de faire confiance à une fonction. Idem découvert pour un identifiant non déclaré avant son
  point d'usage (`PLAYER_posX_i_hi`) : accepté silencieusement par ce compilateur ancien (implicite/
  tardif), toujours corrigé proprement plutôt que laissé tel quel.
- Total bo3 réel : **48/70** (35/54 zone1, 13/16 zone2).
- [x] `8019D768` (dernière du trio "clipping 2D") : voir section dédiée ci-dessous — traitée et
      vérifiée le même jour.

## 2026-07-16 — bo3 : `func_us_8019D768`, fin du trio "clipping 2D" : 49/70

La plus complexe des trois fonctions de clipping (`8019D1D4`/`8019D51C`/`8019D768`) — double
clipping en cascade plutôt qu'un seul niveau :

1. Clip standard contre le viewport caméra (comme les deux précédentes), table
   `D_us_80180A1C` (stride 8 : `leftX`/`rightX`/`topY`/`bottomY`, bords directs cette fois, pas
   `left+width`), indexée par `params & 0xFF` sur `params >> 8` entrées.
2. **Reclippée une seconde fois** contre une "fenêtre intérieure" dérivée de `scrollY.i.hi >> 2`
   (arrondi vers zéro, idiome classique confirmé deux fois séparément dans l'asm : une fois pour
   la phase U horizontale, une fois pour cette fenêtre verticale) — un effet de parallaxe à vitesse
   réduite (aperture qui défile 4x plus lentement que la caméra), plausible pour des rais de lumière
   sous-marine visibles à travers une ouverture qui bouge plus lentement que le premier plan.
3. Teinte aussi l'ambiance de fond à chaque appel : `g_GpuBuffers[0/1].draw.r0/g0/b0` = bleu foncé
   fixe — même mécanisme déjà utilisé ailleurs dans le code base pour des fonds d'arrière-plan
   (`src/st/lib/e_library_bg.c`), confirmant qu'il s'agit bien d'un effet d'ambiance.
4. U enroule modulo 126, V modulo 70 (les deux idiomes `multu`/`mfhi`/décalages déjà reconnus
   correctement par `m2c` en `%`, gardés tels quels comme pour `8019D1D4`), décalés par une base
   partagée globale unique `D_us_80180820` (pas par table comme les fonctions précédentes).
   Détail confirmé directement sur l'asm (pas juste `m2c`) : **U repart à zéro (la base partagée) à
   chaque nouveau morceau après le premier** — contrairement à `8019D1D4` où le U continuait après
   le point d'enroulement précédent. La hauteur/V, à l'inverse, sont recalculées identiquement à
   chaque itération de la boucle de découpe en largeur mais donnent toujours le même résultat
   (aucune de leurs entrées ne change dans la boucle) — gardées recalculées en boucle plutôt que
   remontées en dehors, pour rester au plus près de la forme probable du code source d'origine.
5. Couleur de chaque quad : une onde triangulaire symétrique sur les 6 bits bas de `g_Timer` (cycle
   de scintillement sur 256 frames, r/b oscillent entre -0x80 et -0x41, g fixe à 0x80) — confirmé
   directement sur l'asm.

`m2c` avait produit une sortie déjà bien structurée (pas de gotos, contrairement à `8019D51C`) mais
avec un typage confus sur certains registres réutilisés pour deux calculs différents (ex. le
registre "a0" sert successivement à `viewLeft` ajusté puis à un calcul totalement indépendant lié à
`scrollY`) — decodé à la main instruction par instruction avant de retyper proprement.

**Vérifié !** `make build -j` WSL : `✅ us`, byte-exact du premier coup malgré la complexité, aucun
nouveau warning (seuls les warnings pré-existants habituels du fichier).

Le trio "clipping 2D" mis de côté au lot 7 est maintenant entièrement traité et vérifié.

- Total bo3 : **49/70** (36/54 zone1, 13/16 zone2).
- [ ] Reste 7 fonctions mises de côté (`8019E398`, `801A0A80`, `801A1120`, `801A16E4`, `801A1EE4`,
      `801A3EE0`, `801A42A8`) + 10 bloquées par jumptable (toutes en zone1, zone2 n'en a plus).
      `801A3EE0`/`801A42A8` restent les plus ambitieuses (matrices de rotation 3D / machine à états
      dense) ; les 5 autres n'ont pas encore été creusées en détail cette session.

## 2026-07-16 — bo3 zone2 terminée : `801AB53C`, `801ABF50`, `801AC7E8` : 16/16 (52/70)

Les 3 dernières fonctions de zone2 (`2AFCC.c`) — mises de côté plus tôt car nettement plus
volumineuses que le reste du fichier (405 à 741 lignes d'asm chacune, jump tables à 6/8/10 entrées :
`jtbl_us_8019CBAC`, `jtbl_us_8019CB8C`, `jtbl_us_8019CBD4`) — ont été décompilées à la main cette
session, sans raccourci malgré la taille :

- **`func_us_801AB53C`** : machine à états (8 `step`, sous-dispatch `step_s` à 3 cas) du pickup
  "compagnon" en mode Richter — chute/atterrissage, cooldown avant collecte, dispatch vers les
  helpers de récompense déjà nommés (`func_us_801AB1AC`/`22C`/`420`/`4C4`/`514`/`308`), puis affichage
  d'un cadre qui se resserre autour de l'icône avant disparition.
- **`func_us_801ABF50`** : pickup équipement/accessoire — même squelette (chute, cooldown,
  clignotement, remise à l'inventaire), avec en plus la gestion d'un slot d'icône libre (recherche
  0-31 dans `D_us_801B34F0`) et le rendu de son quad d'icône (atlas UV/CLUT calculé depuis le slot).
  Bascule vers `func_us_801AB53C` si `g_PlayableCharacter` est Richter.
- **`func_us_801AC7E8`** : pickup relique — la plus dense des trois (10 états). Étape 0 alloue une
  chaîne de 7 primitives et fait léviter l'objet (oscillation vitesse Y pilotée par
  `hoverVelocityDelta`) ; étape 5 marque le flag `g_Status` et prépare un décor "rayon" ; étape 6
  rend le nom de la relique en texture via `func_us_801AC538` (déjà décompilée précédemment) ;
  étapes 7/8 animent une ouverture du rayon sur 16 frames (`(t*120)/11`, idiome de division
  réciproque GCC standard) ; étape 9 referme puis détruit l'entité après ~97 frames.

**Bug trouvé par recoupement croisé** (les 3 fonctions partagent la même logique d'atterrissage) :
`func_us_801ABF50` avait été écrite avec `if (!(col.effects & 5)) FallEntity(); else if (velocityY >
0) { ...atterrit... }` — ne faisant **rien** dans le cas `effects&5` vrai mais `velocityY<=0`.
Repéré en comparant à la forme strictement identique dans `func_us_801AB53C` (qui, elle, avait la
bonne condition `!(effects&5) || velocityY<=0`) ; corrigé dans les deux fonctions.

Autres corrections faites en écrivant ces trois fonctions (avant tout commit, donc jamais vues par un
vrai build) :
- `D_us_801818C0` était un tableau `u16`, alors que l'asm fait `sll ,2` + `lw` (32 bits) : c'est un
  tableau de pointeurs, pas de `u16`.
- Les extern forward-declarations pré-existantes de `func_us_801AB53C`/`801ABF50` (dans
  `func_us_801AD2B8`, écrites lors d'une session antérieure avant que ces deux fonctions existent
  réellement) leur donnaient une signature à 3 arguments `(Entity*, u32, Entity*)` — la vraie
  signature, confirmée par l'asm, n'en prend qu'un seul (`Entity* self`). Aurait cassé la compilation
  dès l'ajout des vraies définitions ; corrigé.
- `g_unkGraphicsStruct.unk18`/`.unk1C` : ces noms de champs existent bien dans le struct mais **ne
  vivent pas à l'offset que leur nom suggère** (décalés de 8 octets par des champs ajoutés plus tôt
  dans le struct sans renommage) — les vrais champs à ces offsets s'appellent
  `BottomCornerTextTimer`/`BottomCornerTextPrims`. Bug déjà présent dans du code antérieur
  (`func_us_801AB22C`) en plus du nouveau code ; corrigé aux deux endroits.
- Code mort transcrit tel quel (pas supprimé) dans `func_us_801AC7E8` étape 5 : un test
  `itemId < 0x14` imbriqué dans un bloc déjà gardé par `itemId >= 0x16`, donc mathématiquement
  inatteignable — gardé avec un commentaire explicite plutôt que "corrigé" sans preuve.

**Vérification faite cette session** : compilation isolée (`gcc -fsyntax-only -std=gnu99`, hors
toolchain PSX réelle) sur le fichier `2AFCC.c` complet (16/16 fonctions, ~1400 lignes) — **exit code
0, aucune erreur**, seulement les warnings déjà connus/pré-existants du fichier (mismatch
pointeur/tableau sur `g_Pix`, cf. sessions précédentes). **Pas encore vérifié par `make build -j` en
WSL** — contrairement aux lots précédents de bo3, cette vérification octet-exacte reste à faire côté
utilisateur (nécessite la toolchain PSX, absente de ce sandbox).

- Total bo3 : **52/70** (36/54 zone1, **16/16 zone2 — zone2 terminée**).
- [x] **Vérifié !** `make build -j` WSL lancé par l'utilisateur juste après : `✅ us`, build complet
      2546/2546 réussi (byte-exact). Les 3 fonctions de zone2 (`801AB53C`/`801ABF50`/`801AC7E8`) sont
      donc confirmées correctes, pas seulement plausibles. Zone2 de bo3 réellement terminée.
- Note en passant (hors scope zone2) : le build a affiché des warnings `conflicting types` dans
  `1CEEC.c` (zone1, pas touché cette session) — `g_pads`, `D_us_80180588`, `AllocEntity`,
  `AnimateEntity`, `SquareRoot0`, `ET_801A5E4C` redéclarés différemment de `game.h`/plus haut dans le
  fichier. Non bloquant (le compilateur PSX ne lève qu'un warning), build quand même `✅ us` — mais à
  nettoyer un jour pour éviter qu'une vraie erreur de type s'y cache silencieusement. Pas traité cette
  session, zone1 restant hors périmètre.
- [ ] Reste zone1 : 7 fonctions mises de côté + 10 bloquées par jumptable (inchangé, voir entrée
      précédente) + nettoyage des conflicting-types ci-dessus.

## 2026-07-16 — bo3 zone1 : corruption découverte et réparée dans `1CEEC.c` (14 fonctions)

En reprenant le travail sur zone1 (fixer le blocage jumptable, tâche suivante prévue), une
vérification `gcc -fsyntax-only` sur `1CEEC.c` seul (jamais faite isolément jusqu'ici — seul le
build WSL complet avait été utilisé comme signal) a révélé une corruption bien plus étendue que ce
que l'entrée précédente ("incident méthodologique") laissait supposer :

- **11 fonctions étaient purement et simplement absentes du fichier** (`8019CEEC`, `801A07CC`,
  `801A16E4`, `801A1BE8`, `801A1EE4`, `801A2AEC`, `801A365C`, `801A3EE0` + 3 autres) : leur
  `INCLUDE_ASM` avait été remplacé par le texte brut d'un message d'échec de `decompile.py` (parfois
  nommé explicitement la fonction, parfois un message générique non lié référençant une fonction
  totalement différente, `func_us_801A8D7C` — probablement une erreur de contexte lors d'un run en
  lot, collée au mauvais endroit). Le fichier ne compilait donc plus du tout tel quel.
- **3 fonctions comptées comme "décompilées" (`8019E398`, `801A0A80`, `801A1120`) étaient en réalité
  des dumps `m2c` bruts jamais retypés** : variables `arg0`/`temp_v0`/`var_a0`/`saved_reg_s0` au lieu
  des champs `Entity` nommés, unions `.ext.ILLEGAL.u16[...]` au lieu de structs `ET_*`, et pour
  `801A1120` littéralement des tokens `?` de type inconnu laissés par `m2c` dans de vraies lignes de
  code (`extern ? g_PrimBuf_0_r0;`) — invalide en C, aucun compilateur (même l'ancien compilo PSX) ne
  tolère ça. `8019E398` utilisait aussi `PLAYER_posY_i_hi`/`PLAYER_posX_i_hi` sans déclaration
  visible avant leur premier usage.

**Comment c'est passé inaperçu** : le seul signal de vérité utilisé jusqu'ici était `make build -j`
en WSL, qui affiche `✅ us` si le byte-exact passe — mais une build complète (2546 fichiers) peut
très bien avoir été lancée contre un état du dépôt antérieur à cette corruption (le script de sync
ne liste que les fichiers différents, pas de confirmation explicite que `1CEEC.c` était bien inclus
dans le lot synchronisé cette fois-là). Leçon retenue : **toujours faire un `gcc -fsyntax-only`
isolé sur le fichier qu'on vient de toucher avant de se fier au seul résultat `✅ us` global**, qui
peut masquer un problème localisé si la synchronisation n'a pas pris.

**Réparation faite** : les 11 fonctions absentes ont retrouvé un `INCLUDE_ASM` propre (position
reconstruite par ordre d'adresse croissante entre les fonctions déjà correctement en place — l'ordre
exact d'origine n'est pas garanti à 100% mais n'a aucun impact sur la compilation/le linkage). Les 3
dumps bruts non retypés ont été **repassés en `INCLUDE_ASM` plutôt que "corrigés en place"** : du
code qui ne compile pas et ignore les conventions du projet (noms de champs, structs `ET_*`) n'est
pas un progrès réel, juste une fausse impression d'avancement — cohérent avec la consigne
"sans bâcler". `gcc -fsyntax-only` sur le fichier complet après réparation : **plus aucune erreur
propre à cette corruption**, ne restent que les 6 `conflicting types` déjà identifiés et documentés
(non bloquants, warning seulement côté vrai compilo PSX).

- **Total bo3 réel, revu à la baisse** : **~38/70** (22/54 zone1 — 14 fonctions retirées du compte
  après cette réparation —, 16/16 zone2). Le chiffre "36/54" des sessions précédentes incluait ces
  3 dumps bruts jamais valides.
- [ ] **18 fonctions à décompiler proprement en zone1** (au lieu de 17 précédemment estimé, l'écart
      vient de cette découverte) : les 10 bloquées jumptable + les 7 mises de côté + `8019E398`
      (méthode déjà éprouvée sur bo1 pour le blocage jumptable : ajouter les labels `.Lus_<adresse>`
      manquants + réécrire les `.word` de la jumptable en labels symboliques avant de repasser par
      `m2c`).
- [x] **Vérifié !** `make build -j` WSL relancé par l'utilisateur juste après la réparation, sortie de
      build relue spécifiquement pour `1CEEC.c` cette fois (pas juste le `✅ us` global) : seuls les 6
      `conflicting types` déjà connus et non bloquants apparaissent (`g_pads`, `D_us_80180588`,
      `AllocEntity`, `AnimateEntity`, `SquareRoot0`, `ET_801A5E4C`) — aucune trace des erreurs liées
      à la corruption (fonctions absentes, tokens `?`, registres non déclarés). Build complet
      2546/2546, `✅ us`. La réparation des 14 fonctions est donc confirmée correcte, pas juste
      plausible.
- [ ] Reste à faire : décompiler proprement les 18 fonctions de zone1 (10 bloquées jumptable + 7 mises
      de côté + `8019E398`), en commençant par le déblocage jumptable (méthode `.Lus_<adresse>` déjà
      éprouvée sur bo1).

## 2026-07-17 — bo3 zone1 : premier déblocage jumptable, `func_us_8019CEEC` : 23/54

**Découverte importante sur `tools/decompile.py`** : le script écrit TOUJOURS le résultat dans le
fichier source cible, succès ou échec confondus — en cas d'échec (jumptable manquante, erreur de
contexte), le texte d'erreur lui-même est injecté à la place de l'`INCLUDE_ASM`, écrasant le stub.
C'est très probablement la cause profonde de la corruption réparée hier : quelqu'un (ou un script)
a fait tourner `decompile.py` sur des fonctions bloquées et le résultat s'est retrouvé committé tel
quel. **Confirmé par accident cette session** : un test en boucle sur les 18 fonctions restantes
(pour sonder leur statut) a re-corrompu exactement les 8 mêmes fonctions avant d'être interrompu par
un timeout — réparé une seconde fois avec la même méthode. **Règle désormais actée : ne plus jamais
lancer `decompile.py` directement contre le dépôt réel.** Tout run se fait dans une copie isolée
(`/tmp/repo_iso2`, avec `tools/m2c` + `tools/decompile.py` + `tools/m2ctx.py` copiés en plus
d'include/config/asm/src), et seul le résultat retypé à la main est ensuite appliqué au dépôt réel
via édition ciblée — jamais un `cp`/écrasement direct du fichier source par le script.

**Méthode de déblocage jumptable, appliquée avec succès pour la première fois sur bo3** (déjà
éprouvée sur bo1, jamais réessayée ici avant) : dans une copie isolée du `.s` de la fonction, ajouter
les labels `.Lus_<adresse>` manquants aux points ciblés par la jumptable (4 sur 5 adresses cibles de
`jtbl_us_8019C9C4` n'avaient pas de label), puis ajouter un second bloc `glabel jtbl_us_8019C9C4` /
`.word .Lus_<label>` (au lieu des adresses hexadécimales brutes) directement à la fin du même fichier
`.s` — pas besoin de toucher `asm/us/boss/bo3/data/0.data.s` du dépôt réel, cette rustine ne sert qu'à
faire avaler le switch à `m2c` dans la copie isolée. Une fois la vraie fonction en C, ce fichier `.s`
redevient une simple référence historique.

`func_us_8019CEEC` (dispatcher spawn/init d'objet destructible, `subtype = params >> 0xC`) décompilée
et retypée à la main à partir du dump `m2c` brut obtenu ainsi :
- 9 tables parallèles indexées par `subtype` (blendMode, hitboxHeight, animSet, unk5A, palette,
  hitboxOffY, pointeur d'anim, valeur de reward) — tailles/strides confirmés en lisant directement
  les octets bruts de `asm/us/boss/bo3/data/0.data.s` plutôt que devinés.
- **Bug de `m2c` repéré et corrigé** : le dump brut nommait un champ `g_unkGraphicsStruct.unk10`,
  mais `include/game.h` confirme sans ambiguïté (offsets annotés en absolu) que l'offset `0x10` de
  cette struct s'appelle réellement `g_zEntityCenter` — le même idiome `zPriority = g_zEntityCenter -
  0x14` déjà vu et vérifié dans `func_us_801AB53C` (zone2). `m2c` a visiblement mal résolu ce nom de
  champ dans son contexte reconstruit ; corrigé en se fiant à `game.h` plutôt qu'au dump.
- Champ `unk30` du dump brut (sur une entité fraîchement allouée, type non résolu par `m2c` car issue
  d'un retour `void*`) corrigé en `params` (offset confirmé 0x30 dans `Entity`, cf. table d'offsets
  déjà établie).
- Deux nouveaux symboles déclarés localement : `func_us_801A8D7C(s32 type, Entity* newEntity)`
  (famille du dispatcher `func_us_801A8DF0`, toujours un stub), `g_Entities_160[]`.

Vérifié par `gcc -fsyntax-only` isolé sur le fichier complet : aucune nouvelle erreur, seuls les 6
`conflicting types` déjà connus/non-bloquants restent.

**Statut vérifié précisément cette session** (chaque fonction testée individuellement via
`decompile.py` dans la copie isolée, pas supposé) : sur les 17 fonctions restantes de zone1, exactement
**9 sont bloquées jumptable** (`801A07CC`, `801A1BE8`, `801A2AEC`, `801A365C`, `801A4680`, `801A5948`,
`801A5F0C`, `801A6370`, `801A6568`) et **8 se décompilent directement via `m2c`** mais restent des
dumps bruts non retypés (`8019E398`, `801A0A80`, `801A1120`, `801A16E4`, `801A1EE4`, `801A3EE0`,
`801A42A8`, `801A5338`).

`func_us_801A07CC` débloquée et décompilée dans la foulée (même méthode, jumptable
`jtbl_us_8019C9E4`, 6 labels manquants sur 7 ajoutés). Machine à états (6 `step`) qui fait
monter/descendre le niveau d'eau de la salle de Scylla (`D_us_80180C64`, déjà nommé "scrollY" dans
`func_us_801A071C` — confirme le lien) au rythme d'1 unité toutes les 2-3 frames, en signalant la
direction via `D_us_801B2870` (delta partagé consommé par d'autres entités flottant à la surface,
cf. `func_us_801A1120`) et en resynchronisant les tables de limites de scroll via
`func_us_801A071C()`. Départ/arrêt du mouvement piloté par la position X du joueur et un flag
`g_CastleFlags[0xC3]` ("déclencher l'inondation").

Vérifié par `gcc -fsyntax-only` isolé : toujours seulement les 6 `conflicting types` déjà connus.

- Total bo3 : **24/54 zone1**. Reste 8 fonctions bloquées jumptable + 8 dumps bruts à retyper.

`func_us_801A1BE8` débloquée et décompilée dans la foulée (jumptable `jtbl_us_8019CA0C`, 6 labels
manquants sur 7). Plateforme montante/descendante liée au niveau d'eau, **partage `D_us_80180C66`
avec `func_us_801A07CC`** : le bit `params` de ce masque indique si la phase de cette plateforme est
active. Machine à 6 états : posY oscille entre le haut et le bas de l'écran selon le niveau d'eau,
avec une danse de collision joueur (`GetPlayerCollisionWith`, décalage de +-8 en X avant/après,
idiome déjà vu ailleurs dans ce fichier) à l'aller comme au retour.

**Piège débusqué en retraçant l'asm au lieu de faire confiance au dump `m2c`** : `func_us_801A1B7C`
(déjà décompilée, confiance "haute" affichée) est en réalité appelée ici avec **2 arguments réels**
(`$a0`=self, `$a1`=colonne) alors que sa signature déjà committée n'en a qu'un seul visible — parce
que `$a0`/`self` n'est jamais lu dans son corps, `m2c` l'a légitimement éludé de la signature générée
à l'origine ET dans ce nouveau site d'appel. Pas une incohérence : le paramètre est mort, l'ignorer
aux deux sites d'appel est correct et cohérent. Découvert en gardant un doute sur le rendu
"(s32) arg0" du dump brut plutôt qu'en le recopiant tel quel — un dump `m2c` qui caste un pointeur
d'entité en `s32` pour un paramètre qui s'appelle "column" ailleurs est un signal d'alarme, pas un
détail à ignorer.

Deuxième piège corrigé en cours de route : le "block_27" partagé par les cases 1 et 5 dans le dump
brut ne fait QUE l'incrément de `step` — seule la case 1 positionne `velocityY = 0x40000` avant d'y
sauter (confirmé en comparant les deux labels cibles réels dans le `.s`, `.Lus_801A1CBC` vs
`.Lus_801A1EB8`, qui ne partagent que la toute fin). Une première version écrite trop vite avait
copié ce `velocityY` sur les deux cas ; corrigée après re-vérification asm-par-asm plutôt que
supposée correcte après un simple `gcc -fsyntax-only` propre (qui ne peut évidemment pas détecter ce
genre d'erreur sémantique).

Vérifié par `gcc -fsyntax-only` isolé : toujours seulement les 6 `conflicting types` déjà connus.

- Total bo3 : **25/54 zone1**. Reste 7 fonctions bloquées jumptable (`801A2AEC`, `801A365C`,
  `801A4680`, `801A5948`, `801A5F0C`, `801A6370`, `801A6568`) + 8 dumps bruts à retyper.

**`801A2AEC` sondée mais mise de côté délibérément** : jumptable débloquée avec succès (même
méthode), mais le dump brut révèle une fonction hors norme — la séquence de défaite multi-parties de
Scylla, qui manipule le tableau `g_Entities` par arithmétique de pointeur brute (`arg0 + 0xBC`,
`+0xB04`, `+0x1490`... des multiples exacts de `sizeof(Entity)==0xBC`, donc en réalité des index de
tableau `arg0[1]`, `arg0[15]`, `arg0[28]` déguisés) combinée à des champs `unkXXX` dont l'offset
dépasse la taille d'une seule `Entity` (ex. `unkE0`, `unk11C`) — c'est-à-dire des accès qui retombent
en fait dans l'entité voisine du tableau, pas des champs de l'entité courante. 361 lignes de C brut,
risque élevé d'erreur d'indexation si retypé à la hâte. **Reportée à une prochaine passe dédiée**
plutôt que bâclée — cohérent avec la consigne de rigueur. Le stub original reste intact dans le
dépôt réel (seule la copie isolée a été modifiée pour la sonder).

`func_us_801A6568` débloquée et décompilée (jumptable `jtbl_us_8019CB10`, 5 entrées, 3 labels
manquants). Petit séquenceur de musique post-combat (arrêt de la musique de victoire, attente,
reprise, éventuellement un second cycle avec une autre piste selon `g_CastleFlags[0xC5]`) — partage
le bit 1 de `D_us_80180C66` avec les fonctions de niveau d'eau déjà traitées.

Vérifié par `gcc -fsyntax-only` isolé : toujours seulement les 6 `conflicting types` déjà connus.

- Total bo3 : **26/54 zone1**. Reste 6 fonctions bloquées jumptable (`801A2AEC` mise de côté,
  `801A365C`, `801A4680`, `801A5948`, `801A5F0C`, `801A6370`) + 8 dumps bruts à retyper.

Enchaîné directement sur deux de plus, même méthode à chaque fois (labels manquants + second
`glabel` symbolique en fin de `.s` isolé) :

- **`func_us_801A6370`** (jumptable `jtbl_us_8019CAF8`, 6 entrées) : sibling de `func_us_801A6568`
  (même idiome exact de séquenceur musical post-victoire), mais piloté par `D_us_80180D48` au lieu
  de `D_us_80180C66`, avec un état supplémentaire (6 au lieu de 5).
- **`func_us_801A4680`** (jumptable `jtbl_us_8019CAA0`, 7 entrées) : projectile "œil" de Scylla.
  Vérifie `D_us_80180D48` bit 1 et `self->flags & 0x100` à CHAQUE appel (avant même le switch de
  step) pour voler en éclats en 5 entités type `0x39` — peut interrompre n'importe quel état. Sinon,
  séquence en 7 états : télégraphe, lancement vers le haut, arc montant/descendant avec un timer
  d'ext réutilisé entre deux states différents (freinage progressif de la vitesse puis onde
  sinusoïdale), attente d'anim, puis vise et fonce sur le joueur (angle via `ratan2`/`rcos`/`rsin`
  contre `PLAYER_pos`). Petit détail cross-vérifié dans `include/game.h` plutôt que deviné :
  `arg0->unk50` du dump brut est en fait le champ déjà nommé `pose` (offset 0x50 confirmé).

Vérifié par `gcc -fsyntax-only` isolé à chaque étape : toujours seulement les 6 `conflicting types`
déjà connus (plus une occurrence de plus d'`AnimateEntity`, même warning non bloquant, pas un
nouveau problème).

- Total bo3 : **28/54 zone1**. Reste 4 fonctions bloquées jumptable (`801A2AEC` mise de côté,
  `801A365C`, `801A5948`, `801A5F0C`) + 8 dumps bruts à retyper.

**`func_us_801A5F0C`** (jumptable `jtbl_us_8019CAE4`, 5 entrées, débloqué + décompilé) : débris/éclat
de Scylla à la mort. step 0 choisit un look par sous-type (`params & 0xF`) dans une table
`D_us_8018105C` et fait avancer `step` à `sous-type+1` immédiatement ; `params & 0xFF00` peut
surcharger `zPriority`, et `params & 0xF0` force un flash spécial (palette 0x819F, blendMode 0x10,
facingLeft) qui écrase le look choisi juste avant — les deux chemins retournent (le "default" du
dump brut n'est qu'un point de retour partagé, pas un cas non géré). step 1 est une petite machine à
état interne (`step_s`) : 0 lance la vitesse via rcos/rsin puis tombe dans le comportement de 1
(MoveEntity + AnimateEntity qui déclenche DestroyEntity), 1 fait juste ça, et **step_s >= 2 ne fait
rien du tout et retourne** — cette dernière branche est bien réelle dans l'asm
(`.Lus_801A6000`, comparaison à 3 voies) mais **le dump brut de m2c l'avait silencieusement
supprimée**, fusionnée dans le chemin normal. Repéré et corrigé en retraçant les 3 branches
directement dans le `.s` plutôt que de faire confiance au dump. steps 2/3/4 sont trois variantes
quasi identiques de débris/explosion (seules les constantes de vitesse/opacité changent) ; le dump
brut leur attribuait à chacune un faux argument `Random(0xFFFF4000)`-style — cette constante hex est
en fait une valeur `$a0` déjà consommée par l'instruction précédente (stockage direct dans
velocityY), pas un vrai argument de `Random()` (cf. la note existante sur `Random()` déclarée en
K&R). Argument fantôme retiré dans les 3 cas après re-vérification de l'ordre des delay slots.
Vérifié par `gcc -fsyntax-only` isolé : toujours exactement les 6 `conflicting types` déjà connus,
aucune nouvelle erreur, rien signalé sur cette fonction spécifiquement.

`func_us_801A5948` (jumptable `jtbl_us_8019CAC8`, 7 entrées) débloqué en copie isolée, dump brut
obtenu, **mais mis de côté sans retyper**, même décision que `801A2AEC` et pour la même famille de
raison : le dump brut de m2c contient plusieurs accès `arg0->unk-NNNN` à des offsets NÉGATIFS
(`-0x84A`, `-0x8D0`, `-0x8CC`, `-0xB8`, `-0xBC`) par rapport à `self` (un `Entity*`), qui sortent
largement des bornes de la struct `Entity` (0xBC/188 octets). Analyse manuelle de l'asm (`$s1` reste
constant = `self` sur toute la fonction, jamais réassigné) : `-0xBC` et `-0xB8` sont exactement
`-1 * sizeof(Entity)` et `-1 * sizeof(Entity) + 4`, donc `self[-1]->posX.val` / `self[-1]->posY.val`
— l'élément PRÉCÉDENT dans le même tableau `g_Entities`, cohérent avec un boss façon hydre/serpent à
segments de corps chaînés (Scylla) où chaque segment vise le précédent. `-0x8D0`/`-0x8CC` sont
exactement `-12 * sizeof(Entity)` et `+4`, donc `self[-12]->posX.val/posY.val` — un ancrage fixe
12 emplacements plus loin (probablement la "tête"/l'entité d'ancrage du serpent). `-0x84A` retombe
dans `self[-12]` à un offset relatif de `+0x86`, à l'intérieur de son `ext` — un champ d'état
custom de l'ancrage. Techniquement compréhensible, mais engager un retype complet demanderait de
confirmer précisément ce qu'est "l'ancrage à -12" et son champ ext+0xA sans aucune fonction sœur déjà
faite dans ce fichier pour comparer — risque d'erreur silencieuse trop élevé pour aller vite.
Reporté à une passe dédiée, comme `801A2AEC`, plutôt que de deviner.

- Total bo3 : **29/54 zone1**. Reste 3 fonctions bloquées jumptable, toutes mises de côté pour une
  passe dédiée (`801A2AEC` 783 lignes, `801A5948` 342 lignes, `801A365C` 453 lignes pas encore
  examinée) + 8 dumps bruts à retyper (ceux-là n'ont pas de jumptable à débloquer, juste du retypage
  mécanique -- prochaine cible logique).

Basculé sur les 8 dumps bruts, deux faits, un mis de côté :

- **`func_us_8019E398`** (~167 lignes) : effet visuel de "vague/tremblement" -- 4 quads GT4 alloués
  une fois, retiles chaque frame en bande verticale qui suit le scroll de la caméra, répartie sur
  deux pages de texture (0xE et 0xF, la seconde via un wrap modulo 126 -- texture source plus haute
  qu'une seule page). Son positionnel (volume/pan) calculé via la formule standard position-relative-
  au-joueur déjà vue ailleurs dans ce fichier. m2c avait déjà correctement résolu tous les champs
  `Primitive*` (pas de renommage nécessaire) ; seul nettoyage : remplacer le `goto` de fusion step0→
  code commun par un simple `if`.
- **`func_us_801A16E4`** (~243 lignes asm) : effet "ondulation d'eau" -- 2 quads GT4 empilés qui
  suivent la montée/descente du niveau d'eau via le même global `D_us_801B2870` que
  `func_us_801A07CC`, et qui s'estompent vers le noir jusqu'à s'auto-détruire. Ici le dump brut de
  m2c n'avait PAS résolu les champs `Primitive*` (tout en `void* + unkNN`), donc chaque offset a été
  re-vérifié à la main contre `include/primitive.h` avant renommage. Point le plus délicat : `ext+0x8`
  contient à la fois un compteur fractionnaire ET la hauteur d'ondulation réelle dans le même mot de
  32 bits (`ext.i.lo`/`ext.i.hi`, exactement le même idiome que le type `f32` déjà utilisé partout
  pour posX/posY) -- confirmé en re-traçant l'asm `.s` directement plutôt que de faire confiance aux
  index `u32[2]`/`u16[5]` du dump qui, pris littéralement, se chevauchaient (signe que le dump avait
  besoin d'être vérifié, pas copié tel quel). L'expression brute de calcul du pointeur de primitive
  (arithmétique en plusieurs étapes) a été simplifiée algébriquement en `&g_PrimBuf[allocIdx]` après
  avoir confirmé que la constante se réduisait exactement à `sizeof(Primitive)`.
- **`func_us_801A0A80`** mis de côté sans retyper : dump très risqué (état persistant entre appels via
  des registres `$s6`/`$s7` jamais initialisés dans la fonction elle-même -- `saved_reg_s6/s7` --,
  plus un slot de sortie `sp12` utilisé par le dump sans jamais être déclaré). Dessine un HUD de debug
  (`FntPrint`, `g_unkGraphicsStruct.D_80097448/4C/50`) avec détection de collision par balayage de
  distance. Risque d'erreur silencieuse trop élevé pour aller vite -- reporté avec les 3 autres
  fonctions complexes déjà mises de côté.

Vérifié par `gcc -fsyntax-only` isolé après chaque fonction retypée : toujours exactement les 6
`conflicting types` déjà connus, zéro nouvelle erreur.

**`func_us_801A1120`** et **`func_us_801A1EE4`** faites (famille "débris/éclaboussure d'impact dans
l'eau" de Scylla, même step-machine à 3 états : init+alloc GT4, chute avec gravité tabulée
`D_us_80180C80[subtype]` (vitesse initiale + gravité, en réalité UN SEUL tableau à 2 champs 8 octets
-- confirmé directement dans `asm/us/boss/bo3/data/0.data.s`, `D_us_80180C84` n'est pas un symbole
distinct mais commence exactement 4 octets après `D_us_80180C80` -- fusionnés en un seul type ici),
puis fondu au noir jusqu'à auto-destruction). Le dump brut signalait un `saved_reg_s0` façon
"état persistant entre appels" comme pour `801A0A80`, mais ici c'est un faux positif : c'est
simplement le pointeur de la primitive précédente dans la boucle, toujours écrit à la 1ère itération
avant d'être lu à la 2e -- vérifié en retraçant la boucle, pas une vraie dépendance inter-appels.
Bug de renommage repéré et corrigé sur `801A1120` : le champ `ext.prim` (utilisé comme un `s32`
"gravité", pas un pointeur) est réellement à `ext+0x0`, pas `ext+0x4` comme je l'avais d'abord
écrit -- corrigé après re-vérification directe des offsets absolus dans le `.s`.
`801A1EE4` est le même mécanisme mais avec 2 quads GT4 optionnels en plus (un "connecteur" qui relie
le bord étiré du quad0 vers le point d'impact, incliné via une table de pente `D_us_80180D38`) --
retypé par comparaison directe avec `801A1120` déjà vérifié, avec une erreur de branchement
repérée et corrigée en cours de route (un `else if` à 3 voies que j'avais inventé pour la logique
d'inclinaison négative alors que le dump n'en a que 2).

Vérifié par `gcc -fsyntax-only` isolé après chaque fonction : toujours exactement les 6
`conflicting types` déjà connus, zéro nouvelle erreur.

- Total bo3 : **33/54 zone1**. Reste 3 fonctions bloquées jumptable (mises de côté) + 1 dump brut
  mis de côté (`801A0A80`) + 3 dumps bruts encore à retyper (`801A3EE0`, `801A42A8`, `801A5338`).

**Correction d'étiquetage (2026-07-17)** : ce boss (`bo3`) est **Scylla**, pas Scylla. Vérifié via
`config/splat.us.bobo3.yaml` (`target_path: disks/us/BOSS/BO3/BO3.BIN`) contre le tracker public
https://sotn.xee.dev/ qui liste `BOSS/BO3/BO3.BIN` comme "Boss: Scylla" (Scylla est `BO7.BIN`). Tous
les effets d'eau (ondulations, vagues, éclaboussures) qu'on décompile ici ont du sens pour Scylla : sa
salle a un bassin dans lequel il plonge pendant le combat. Toutes les mentions "Scylla" de ce fichier
et de la mémoire ont été corrigées en "Scylla" (renommage cosmétique uniquement, aucun code changé).

**`func_us_801A3EE0`** faite : projectile "disque héraldique" tournant d'Scylla. Avorte immédiatement
si le bit 1 de `D_us_80180D48` est levé (même flag partagé "boss vaincu/interruption" que
`801A16E4`/`801A6568`/`801A6370`). Intro en 2 steps (délai de 0x20 frames puis grossissement
scaleX/Y), attend un flag sur l'entité "parent" (Scylla lui-même, pointeur stocké en `ext+0x20`, posé
par quelqu'un d'autre) avant de se lancer sur le joueur ; steps 2/3 allouent 2 quads GT4 et animent
une vraie rotation 3D via la GTE (`RotMatrix`/`RotMatrixZ`/`RotMatrixX`/`RotAverage4`, déjà bien
typés par m2c grâce à `include/psxsdk/libgte.h`), puis s'auto-détruit en sortant de l'écran. Seul
vrai risque de cette fonction (le pointeur `ext+0x20` vers l'entité parent et son flag `+0x9`) reste
de sémantique inférée par l'usage, pas confirmée par une source nommée -- documenté comme tel.
Vérifié par `gcc -fsyntax-only` isolé : toujours les 6 mêmes symboles connus (une occurrence de plus
d'`AnimateEntity`, même redéclaration locale déjà vue ailleurs dans ce fichier, pas un nouveau
problème).

**`func_us_801A42A8`** faite : attaque "jet d'encre/acide" arqué d'Scylla. Alloue une chaîne entière
de primitives via `g_api_func_800EDB58` (même idiome que `func_us_801A27EC` déjà dans ce fichier),
les peint en bleu foncé et les cache, puis lance le projectile sur une trajectoire balistique vers
une cible X fixe (0x5C) avec un calcul de "temps jusqu'à l'impact" qui recalcule `velocityY` une
fois en vol pour atterrir pile sur une cible Y (0x54) au moment où le minuteur expire. Pendant la
chute, le segment de trace courant est étiré vers la position actuelle chaque frame, et un nouveau
segment démarre tous les 4 frames (chaînage). Une fois posé, chaque segment s'estompe via
`func_us_801A2A90(prim, 1)` -- fonction déjà décompilée plus haut dans ce même fichier, donc pas de
devinette sur son rôle -- et l'entité s'auto-détruit une fois tous les segments éteints. Argument
fantôme `Random(0x98)` retiré (même schéma déjà vu : delay slot vide avant le `jal Random`).
Vérifié par `gcc -fsyntax-only` isolé : zéro nouvelle erreur/warning au-delà des 6 symboles connus.

**`func_us_801A5338`** examinée puis **mise de côté**, même famille de risque que `801A2AEC` et
`801A5948` : c'est en fait la fonction qui CRÉE toute la chaîne de segments corporels d'Scylla (12
parties au total, `arg0 + N*0xBC` avec `N=1..11`, chaînées via les champs `parent`/`nextPart` --
ceux-là sont de vrais champs déjà établis dans `include/game.h`, ligne 934-935, "for multi-part
entities only", ce qui confirme la lecture "créature à segments"). Mais le dump brut contient aussi
des accès `arg0->unk900`/`unk92C`/`unk930`/`unk8F4` -- des offsets bien plus grands que la struct
`Entity` (0xBC octets), qui ne sont pas des multiples propres de `0xBC` non plus (contrairement aux
+0xBC/+0xB04/+0x1490 de `801A2AEC`), donc probablement des adresses de globales calculées via le
même genre d'astuce "adresse d'entité connue ± constante" que `801A5948` -- à élucider en comparant
avec `asm/us/boss/bo3/data/0.data.s` avant de risquer un mauvais renommage. Vu que c'est littéralement
la fonction qui construit la structure que `801A5948`/`801A2AEC` consomment, une erreur ici se
propagerait partout -- reportée à la même passe dédiée que les 3 autres.

- Total bo3 : **35/54 zone1**. Les 8 dumps bruts du lot initial sont tous traités : 6 retypés
  (`8019E398`, `801A16E4`, `801A1120`, `801A1EE4`, `801A3EE0`, `801A42A8`), 2 mis de côté
  (`801A0A80`, `801A5338`) pour la même raison que les 3 jumptables déjà réservés. Reste donc pour la
  passe dédiée "complexe" : `801A2AEC` (783 lignes), `801A5948` (342 lignes), `801A365C` (453 lignes,
  jamais encore examinée), `801A0A80`, `801A5338` -- 5 fonctions au total, 54-35-5=14... à vérifier
  précisément avant la prochaine session (mes comptes de "reste" ont pu glisser en cours de route).
- [x] Vérification WSL réelle faite (2026-07-17) : `psx cc src/boss/bo3/1CEEC.c` compile avec
      exactement les 6 avertissements `conflicting types` déjà connus (`AllocEntity`, `g_pads`,
      `D_us_80180588`, `AnimateEntity`, `SquareRoot0`, `ET_801A5E4C`), aucun nouveau, et le build
      complet se termine par `✅ us`. **Confirme que le lot entier (les 6 fonctions retypées
      aujourd'hui + les 7 jumptables déjà faites) est réellement octet-exact**, pas seulement
      syntaxiquement propre. Zone1 est donc solidement à 49/54.
- [ ] Passe dédiée sur les 5 fonctions complexes mises de côté (`801A5338` en premier -- c'est elle
      qui construit la chaîne de segments corporels que `801A2AEC`/`801A5948` consomment), en
      élucidant d'abord les offsets "globale déguisée en champ d'entité" via `0.data.s` avant de
      retyper quoi que ce soit.

## bo3.h / e_init.c -- câblage de la table de dispatch d'entités (2026-07-17, suite)

**Correction de périmètre importante.** L'hypothèse de début de session ("109 stubs restants sur les
14 autres fichiers bo3 = pure migration mécanique, zéro décompilation") était fausse pour une partie
du lot. En inspectant le contenu réel de chaque fichier (pas juste son nom), 5 des 14 fichiers
contiennent du code réellement propre à Scylla sous des noms bruts `func_us_XXXXXXXX` -- les migrer
en bloc aurait supprimé silencieusement du vrai comportement de jeu :
- `e_room_bg.c` et `e_lock_camera.c` sont 100% orphelins (`func_us_8019CC68`, `func_us_8019CD30`).
- `e_red_door.c` et `create_entity.c` sont des mélanges (nom partagé + du code bo3-only).
- `e_misc.c` est le plus mélangé : contient **`EntityScyllaDrool`**, littéralement nommée d'après ce
  boss, plus 4 `func_us_` orphelins, `EntityBigRedFireball`, 2 `UnkRecursivePrimFunc`.

Ces 5 fichiers sont réservés à une vraie passe de décompilation (comme n'importe quelle autre
fonction du projet), pas à une migration.

**`bo3.h` et `e_init.c` créés**, avec la table de dispatch `PfnEntityUpdate
BO3_EntityUpdates[]` (68 entrées, 0x00-0x43) retranscrite octet par octet depuis
`asm/us/boss/bo3/data/0.data.s` (`glabel D_us_80180424`), vérifiée par calcul d'adresse précis
(`(offset - 0x424) / 4`) plutôt que comptage manuel (une première tentative manuelle avait un
décalage d'un cran, corrigé en recalculant chaque adresse). `bo3.h` contient l'enum `EntityIDs`
(noms partagés là où ils existent, `E_UNK_XX` + commentaire du vrai nom de fonction sinon) et le
macro `OVL_EXPORT`.

**9 fichiers initialement identifiés comme "sûrs"** (chaque stub correspond exactement à un symbole
du moteur partagé) ont été réduits à **4 réellement câblés cette session** : `st_common.c`,
`prim_helpers.c`, `st_update.c`, `popup.c` -- aucun n'a besoin d'un `EInit` dont la vraie valeur
compilée n'a pas pu être localisée. Les 5 autres (`e_life_up.c`, `e_particles.c`, `e_room_fg.c`,
`e_stage_name.c`, `st_collision.c`) ont chacun besoin d'un tuple `EInit` (`g_EInitCommon`,
`g_EInitInteractable`, `g_EInitDamageNum`, `OVL_EXPORT(EInitObtainable)`) dont la vraie valeur/
adresse compilée n'a pas pu être confirmée dans le blob de données encore non découpé -- deviner
risquait soit une valeur silencieusement fausse, soit une collision de symbole avec les vrais
octets déjà présents. Laissés en `INCLUDE_ASM` sous leur nom réel (partagé) : la table de dispatch
les référence correctement quelle que soit leur forme.

**Découpage de segment splat** : la table `D_us_80180424` vivait dans un immense blob `data` non
identifié (`[0x0, data]`, 0x1CC68 octets). Ajout d'un sous-segment `[0x424, .data, e_init]` dans
`config/splat.us.bobo3.yaml` (type `.data`, pas `c` -- premier essai avec `c` a fait désassembler
la table comme instructions MIPS bidon, corrigé après coup). Testé via `splat split` en isolation
(le disque réel + `splat64` sont disponibles dans ce projet) : split propre, aucune erreur, les
fichiers source ne sont pas touchés, et `asm/us/boss/bo3/data/e_init.data.s` généré confirme
byte-exact la même table que celle écrite dans `e_init.c`.

**Vérifié** : `gcc -fsyntax-only` propre sur `bo3.h`, `e_init.c` et les 4 fichiers migrés ; `1CEEC.c`
ne montre toujours que les 6 `conflicting types` déjà connus (aucune régression).

**Pas encore fait** : vrai build WSL pour confirmer octet-exact sur ce lot ; la localisation des 4
valeurs `EInit` réelles (pour migrer les 5 fichiers restants) ; la décompilation des 5 fichiers
réellement Scylla-spécifiques.
2026-07-17

## Premier vrai build WSL après le câblage : 2 catégories d'erreurs de link, corrigées (2026-07-17)

Le vrai build (`make build -j`) a échoué au link de `bobo3.elf`, exactement comme redouté --
mais avec des erreurs précises et faciles à corriger, pas une corruption silencieuse :

1. **`undefined reference to D_us_80180424`** dans `create_entity.c` et `e_misc.c` (tous deux
   encore de l'asm brut non migré). Cause : j'avais nommé mon tableau `BO3_EntityUpdates` (via
   `OVL_EXPORT`) dans `e_init.c`, mais ces deux fichiers référencent encore la table sous son
   ancien nom brut assigné par splat, figé dans leur `.s`. **Fix** : renommé le tableau en
   `e_init.c` pour qu'il s'appelle littéralement `D_us_80180424` (pas de wrapping OVL_EXPORT) --
   à renommer proprement seulement quand ces deux fichiers seront eux-mêmes migrés.
2. **`undefined reference to EntityExplosion/EntityPrizeDrop/EntityEquipItemDrop`** dans
   `st_common.c`. Cause : `st_common.h` assigne ces 3 noms génériques directement à
   `entity->pfnUpdate` (en contournant la table de dispatch par ID) dans
   `EntityExplosionSpawn`/`ReplaceBreakableWithItemDrop` -- mais Scylla a ses propres implémentations
   sous des noms différents (`func_us_801ABDB0`/`801AB53C`/`801ABF50`, déjà décompilées dans
   `2AFCC.c`). **Fix** : 3 `#define` alias + externs dans `bo3.h` (`EntityExplosion` →
   `func_us_801ABDB0`, etc.) -- même signature, même vraie fonction, zéro risque.

Les deux corrections sont appliquées (`bo3.h`, `e_init.c`), re-vérifiées en gcc isolé (0 erreur sur
les 6 fichiers), et le découpage splat re-testé (toujours propre). **Prochain build WSL en attente
pour confirmer.**

## Build WSL confirmé octet-exact après resync (2026-07-17)

Le premier échec était juste un oubli de resync (le `make build -j` avait été relancé sans repasser
par `tools/sync-to-wsl.sh`, donc WSL avait encore l'ancienne version d'avant les 2 fixes). Après
resync : build complet, `1CEEC.c` ne montre que les 6 avertissements `conflicting types` déjà connus
(aucune régression), zéro erreur de link, **`✅ us`**.

**bo3.h/e_init.c + le câblage de la table de dispatch (68 entrées) + les 4 fichiers migrés
(`st_common.c`, `prim_helpers.c`, `st_update.c`, `popup.c`) sont donc confirmés octet-exact.**

Prochaines étapes (voir mémoire projet) : localiser les 4 vraies valeurs `EInit` pour migrer les 5
fichiers restants (`e_life_up.c`, `e_particles.c`, `e_room_fg.c`, `e_stage_name.c`,
`st_collision.c`) ; décompiler à la main les 5 fichiers réellement propres à Scylla (`create_entity.c`,
`e_misc.c` incl. `EntityScyllaDrool`, `e_red_door.c`, `e_room_bg.c`, `e_lock_camera.c`) ; la passe
dédiée sur les 5 fonctions complexes de zone1 (`801A5338` en premier).

## 2 fichiers de plus migrés : e_room_bg.c et e_lock_camera.c (2026-07-17, suite)

En creusant `func_us_8019CC68` (e_room_bg.c) et `func_us_8019CD30` (e_lock_camera.c) -- les deux
seuls fichiers "100% orphelins" du lot de 14 -- il s'avère que malgré leurs noms bruts, leur LOGIQUE
est identique octet pour octet aux implémentations partagées `EntityBackgroundBlock`
(`src/st/e_room_bg.h`) et `EntityLockCamera` (`src/st/entity_lock_camera.h`). Vérifié précisément :

- `func_us_8019CC68` == `EntityBackgroundBlock` : chaque champ lu (`unk54`→`animSet`,
  `unk24`→`zPriority`, `unk5A`, `unk16`→`palette`, `unk19`→`drawFlags`, `unk18`→`blendMode`,
  `unk34`→`flags`, avec le même garde "if nonzero") correspond exactement au struct `ObjInit2` réel.
  `D_us_80180588` (l'EInit passé à `InitializeEntity`) = `{0,0,0,0,3}` à l'octet près -- confirme
  `g_EInitCommon`.
- `func_us_8019CD30` == `EntityLockCamera` : la logique de switch sur `params` (cas 4/5/6),
  `GetSideToPlayer()`, et les tables `D_us_80180664`/`D_us_8018066C`/`D_us_80180674` correspondent
  **octet pour octet** aux tableaux statiques déjà écrits en dur dans `entity_lock_camera.h`
  (`entityLockCameraHitbox`, `entityLockCameraData`, `entityLockCameraTilemapProps`, branche
  `#else` par défaut). `D_us_8018057C` = `{0,0,0,0,1}` -- confirme `g_EInitLockCamera`.

Les deux migrés : `e_room_bg.c`/`e_lock_camera.c` → `#include "bo3.h"` + le header partagé.
`e_init.c` mis à jour : `BO3_EntityUpdates[]` slots 0x11/0x12 pointent maintenant vers
`OVL_EXPORT(EntityBackgroundBlock)`/`OVL_EXPORT(EntityLockCamera)` (plus les anciens noms bruts,
qui n'existent plus une fois migrés). `g_EInitCommon`, `g_EInitLockCamera` et
`BO3_BackgroundBlockInit[2]` (transcrit octet par octet depuis `D_us_8018061C`) définis dans
`e_init.c`.

**Découpage splat** : 4 nouveaux sous-segments `.data` ajoutés dans `config/splat.us.bobo3.yaml`
(`0x57C`/`0x588`/`0x61C`/`0x664`), chacune correspondant exactement aux plages d'octets vérifiées
ci-dessus.

**Vérifié** : `gcc -fsyntax-only` propre sur les 8 fichiers concernés. **Pas encore vérifié** :
`splat split` (l'environnement d'isolation a eu un problème temporaire de lenteur/copie ce tour-ci,
recopie complète impossible dans le temps imparti) ni un vrai build WSL -- **nécessaire avant de
faire confiance à ce lot**, contrairement aux lots précédents où les deux vérifications ont pu être
faites avant de présenter le résultat.

Progrès réel : 2 fichiers de plus sur les "5 restants après EInit" tombent (il n'en reste que 3
vrais orphelins mixtes : `e_life_up.c` [en fait boss-spécifique, PAS un simple générique --
découvert en lisant sa vraie désassemblure, complexe et différente du générique], `create_entity.c`,
`e_misc.c`), plus `e_red_door.c` (mixte, `EntityIsNearPlayer` migrable + `func_us_801A8EE4`
boss-spécifique).

## e_room_bg.c / e_lock_camera.c confirmés octet-exact après 2 fixs de link (2026-07-17)

Même famille d'erreur que le lot précédent, cette fois :
1. `D_us_80180588` (l'adresse réelle de `g_EInitCommon`) est déjà utilisée par son nom brut dans
   `1CEEC.c` -- renommer cassait le link. Fix : gardé `D_us_80180588` comme nom réel dans
   `e_init.c`, `bo3.h` fait `#define g_EInitCommon D_us_80180588`.
2. `D_us_8018060C`/`D_us_80180610` (pointeurs `animFrames` de `BackgroundBlockInit`) n'étaient que
   des `extern` sans définition -- ces adresses tombaient dans le blob encore non découpé. Fix :
   définis directement avec leurs vrais octets dans `e_init.c`.

Après ces 2 fixs : build complet, `1CEEC.c` garde ses 6 avertissements connus (aucune régression),
**`✅ us`**. `e_room_bg.c`, `e_lock_camera.c`, et le `e_init.c`/`bo3.h` mis à jour sont donc
confirmés octet-exact.

Reste sur bo3 : `g_EInitInteractable`/`g_EInitDamageNum`/`OVL_EXPORT(EInitObtainable)` à localiser
pour migrer `e_particles.c`/`e_room_fg.c`/`e_stage_name.c`/`st_collision.c` ; décompilation à la
main de `create_entity.c`, `e_misc.c`, `e_red_door.c`, `e_life_up.c` ; la passe dédiée sur les 5
fonctions complexes de zone1.

## Les 3 derniers EInit trouvés, 4 fichiers de plus migrés (2026-07-18)

Localisés en lisant les vrais sites d'appel `InitializeEntity` dans les `.s` de
`asm/us/boss/bo3/nonmatchings/` et en décodant les octets réels dans `asm/us/boss/bo3/data/0.data.s` :

- `D_us_80180540` = `EInitObtainable` = `{0x0003, 0, 0, 0, 1}` (utilisé par `EntitySoulStealOrb` /
  `EntityEnemyBlood`).
- `D_us_80180564` = `g_EInitInteractable` = `{0, 0, 0, 0, 5}` (utilisé par `EntityStageNamePopup`).
- `D_us_80180594` = `g_EInitDamageNum` = `{0, 0, 0, 0, 3}` (utilisé par `EntityDamageDisplay`).

Les trois correspondent exactement aux valeurs déjà connues pour mar/rbo3. Avant de les câbler,
grep systématique de `1CEEC.c`/`2AFCC.c` (leçon du lot précédent) : `D_us_80180564` est déjà
référencé ~30 fois dans `1CEEC.c`, `D_us_80180540` l'est dans `2AFCC.c` — les deux gardés sous leur
nom brut dans `e_init.c`, avec alias `#define` dans `bo3.h` (`g_EInitInteractable` /
`BO3_EInitObtainable`). `D_us_80180594` n'a aucune référence existante, déclaré directement en
`g_EInitDamageNum`.

**Fichiers migrés** : `e_particles.c`, `e_room_fg.c`, `e_stage_name.c`, `st_collision.c` — tous vers
`#include "bo3.h"` + leur en-tête partagé (`e_particles.h`, `e_room_fg.h`, `e_stage_name_us.h` sous
garde `VERSION_US`, `collision.h`). Aucun de ces en-têtes ne wrappe ses noms de fonction en
`OVL_EXPORT` (contrairement à `e_room_bg.h`/`entity_lock_camera.h`), donc la table de dispatch dans
`e_init.c` n'a nécessité aucun changement pour ce lot.

**Découpage splat** : la plage `0x534`-`0x61C` (auparavant deux gros blobs `data` génériques) a été
redécoupée en segments `.data` nommés pour les 3 nouvelles adresses (`0x540` → `e_particles`,
`0x564` → `e_stage_name`, `0x594` → `st_collision`), en gardant les zones encore non identifiées
(`0x534`, `0x54C`, `0x570`, `0x5A0`) comme blobs `data` génériques.

**Vérifié** : `gcc -fsyntax-only` propre sur les 5 fichiers concernés (`e_init.c` inclus) plus
`1CEEC.c`/`2AFCC.c` — aucune nouvelle erreur "conflicting types" au-delà du `D_us_80180588` déjà
connu et accepté. **Vrai build WSL confirmé** : `1CEEC.c` garde exactement les 6 mêmes
avertissements baseline (`AllocEntity`, `g_pads`, `D_us_80180588`, `AnimateEntity`, `SquareRoot0`,
`ET_801A5E4C`), aucune nouvelle erreur de link, **`✅ us`**. `e_particles.c`, `e_room_fg.c`,
`e_stage_name.c`, `st_collision.c` sont donc confirmés octet-exact.

Il ne reste sur bo3, comme fichiers non migrables (contenu réellement boss-spécifique) :
`create_entity.c`, `e_misc.c` (avec `EntityOlroxDrool`), `e_red_door.c` (mixte), `e_life_up.c`.
Plus la passe dédiée sur les 5 fonctions complexes de zone1 (`func_us_801A5338` en premier).

## create_entity.c migré : bug de frontière de symboles + reconstruction des tables de layout de salles (2026-07-30) — gcc-clean, UNVERIFIED sur vrai WSL

`create_entity.c` semblait avoir 8 "fonctions orphelines" (`func_us_801A889C/88E8/8940/8A3C/8B50/
8CC8/8D7C/8DF0`) mélangées aux 14 fonctions à noms partagés du fichier. Investigation de leur vraie
désassemblure : chacune fait EXACTEMENT une instruction, et cette instruction est en réalité la
PREMIÈRE instruction de la fonction partagée suivante (`FindFirstEntityAbove`, `FindFirstEntityBelow`,
`CreateEntitiesAbove`, `CreateEntitiesBelow`, `InitRoomEntities`, `UpdateRoomPosition`,
`CreateEntityFromCurrentEntity`, `CreateEntityFromEntity`). Confirmé en comparant avec `bo1` (déjà
décompilé), où ces mêmes fonctions démarrent avec exactement cette instruction en tête — pas de
fonction séparée là-bas. **Root cause** : `config/symbols.us.bobo3.txt` déclarait chacune de ces 8
adresses 4 octets trop tard (ex. `CreateEntitiesAbove = 0x801A8944` au lieu de `0x801A8940`), un vrai
bug de config (pas un artefact du binaire retail). Corrigé : les 8 adresses ramenées à leur vraie
valeur. Aucune des 8 "fonctions" n'existe donc réellement — supprimées de `create_entity.c`, plus
besoin de décompilation à la main pour elles.

Pour migrer le fichier vers le header partagé (`create_entity.h`/`pfn_entity_update.h`), il restait
`OBJ_LAYOUT_HORIZONTAL`/`OBJ_LAYOUT_VERTICAL` (deux tables de 53 pointeurs, une par salle du stage) à
localiser — elles vivaient dans le tout premier blob de données de bo3 (`0x0`-`0x424`, jamais
identifié), qui s'est avéré être une instance de la struct `Overlay`/`AbbreviatedOverlay` déjà
documentée dans `include/game.h` (en-tête de stage standard : `Update`, `HitDetection`,
`UpdateRoomPosition`, `InitRoomEntities`, `rooms`, `spriteBanks`, `cluts`, `objLayoutHorizontal`,
`tileLayers`, `gfxBanks`, `UpdateStageEntities`). Seul le champ `objLayoutHorizontal` (à `0x1C`,
valeur `D_us_80180280`) était nécessaire ; le reste de cette struct reste non identifié (pas
bloquant).

Décodage de `D_us_80180280`/`D_us_80180354` (53 pointeurs chacune, confirmé contre la vraie
désassemblure d'`InitRoomEntities`, indexée par `objLayoutId*4`) : la quasi-totalité des 53 salles
partage UNE table "vide" par défaut (juste les sentinelles `LAYOUT_OBJ_START`/`LAYOUT_OBJ_END`, pas
d'entité scriptée) ; seules ~5 salles ont un layout réel. Toutes les données (8 tables, ~900 octets)
extraites et transcrites octet-par-octet via un script Python (parsing direct des champs hexadécimaux
bruts de `asm/us/boss/bo3/data/0.data.s`, aucune transcription manuelle) — élimine tout risque
d'erreur de copie. Gardées comme blobs `u8[]` bruts (pas de struct `LayoutEntity[]` typée), la
signification par champ n'étant pas nécessaire pour un lien octet-exact ; deux tables (`D_us_80181D70`,
`D_us_80181F34`) contiennent en réalité une liste horizontale ET une sous-liste verticale imbriquée
(pointeur milieu-de-tableau à `+0x46` dans les deux cas — même structure, cohérent).

**Conflit trouvé et résolu** : `D_us_80181E58` (le blob "vide" vertical par défaut) est exactement la
même adresse déjà déclarée dans `e_init.c` comme `extern void D_us_80181E58(Entity* self)` pour le
slot 0x00 de la table de dispatch (jamais réellement appelé). Le binaire retail réutilise cette seule
adresse pour deux rôles. Fix : `D_us_80181E58` défini une seule fois comme donnée (`u8[]`), le slot
0x00 fait `(PfnEntityUpdate)D_us_80181E58` pour préserver la valeur du pointeur sans redéclaration
conflictuelle.

**Vérifié** : `gcc -fsyntax-only` propre sur `create_entity.c`, `e_init.c`, plus tous les fichiers
déjà migrés et `1CEEC.c`/`2AFCC.c` (aucune nouvelle "conflicting types", que le `D_us_80180588`
connu).

**2 erreurs de link trouvées et corrigées au premier essai réel** (même famille que d'habitude) :
1. `D_us_80180280` (`OBJ_LAYOUT_HORIZONTAL`) : le blob de données encore brut de bo3
   (`D_us_80180000`, l'instance de la struct `Overlay`) le référence par son nom brut — gardé sous
   ce nom, aliasé dans `bo3.h` (`BO3_pStObjLayoutHorizontal`/`BO3_pStObjLayoutVertical`).
2. `func_us_801A8D7C`/`func_us_801A8DF0` : `1CEEC.c` (qui n'inclut pas `bo3.h`, fichier antérieur au
   système de migration par headers partagés) les appelait encore sous leurs anciens noms d'orphelins.
   Fix : alias `#define func_us_801A8D7C CreateEntityFromCurrentEntity` /
   `#define func_us_801A8DF0 CreateEntityFromEntity` ajoutés directement dans `1CEEC.c`.

Après ces 2 fixs : build réel confirmé, `1CEEC.c` garde exactement les 6 mêmes avertissements
baseline (aucune régression), zéro erreur de link. **`create_entity.c` est confirmé octet-exact.**

Reste sur bo3, non migrables (contenu réellement boss-spécifique) : `e_misc.c` (avec
`EntityOlroxDrool`), `e_red_door.c` (mixte), `e_life_up.c`. Plus les 5 fonctions zone1 différées.

## e_red_door.c migré : `func_us_801A8EE4` était en fait `EntityRedDoor` générique (2026-07-30) — gcc-clean, UNVERIFIED sur vrai WSL

`e_red_door.c` était classé "mixte" (`EntityIsNearPlayer` migrable, `func_us_801A8EE4` supposé
spécifique au boss). Lecture complète de la vraie désassemblure de `func_us_801A8EE4` (849 lignes) :
structure identique à `OVL_EXPORT(EntityRedDoor)` du header partagé (`src/st/e_red_door.h`) —
`InitializeEntity(g_EInitCommon)`, mêmes offsets de champs, même table de saut à 6 cas (`step`
0-5), 6 appels à `g_api_PlaySfxVolPan` au même endroit, appels `rcos`/`rsin` pour la rotation de la
porte, tout correspond ligne pour ligne. Encore un cas du même schéma que `e_room_bg.c`/
`e_lock_camera.c` : nom orphelin en apparence, logique en réalité 100% partagée avec données
locales au boss.

Deux tables locales nécessaires :
- `g_eRedDoorUV` (`D_us_801814E4`, 24 octets) : static déclarée directement dans `e_red_door.h` —
  ses vraies valeurs (`B1 B7 B1 B7 21 21 5F 5F` / `88 A8 88 A8 21 21 5F 5F` / `A8 88 A8 88 21 21 5F
  5F`) correspondent EXACTEMENT aux constantes déjà codées en dur dans le header partagé — rien à
  faire, l'`#include` suffit.
- `RedDoorTiles` (`D_us_80180644`, 32 octets, `u16[2][8]`) : déclarée `extern` dans le header
  (valeurs spécifiques par boss). Suivant la convention déjà utilisée par `mar`/`bo4`/`bo6`/`rbo3`/
  `rbo5`, créé un fichier dédié `e_red_door_tiles.c` avec les 2 lignes réelles transcrites
  (`{1,1,1,1,0,0,0,0}` et `{0x4FA,0x4FA,0x4FA,0x4FA,0,0,0,0}`).

Table de dispatch (`e_init.c`) : slot 0x05 (E_RED_DOOR) renommé de `func_us_801A8EE4` vers
`OVL_EXPORT(EntityRedDoor)` — aucune autre référence brute trouvée ailleurs dans bo3, donc pas
besoin d'alias cette fois.

**Découpage splat** : `[0x644, data]` renommé en `[0x644, .data, e_red_door_tiles]` (32 octets,
correspondait déjà exactement) ; nouveau `[0x14E4, .data, e_red_door]` carvé dans le gros blob
`[0x6E4, data]` pour `g_eRedDoorUV`.

**Vérifié** : `gcc -fsyntax-only` propre sur `e_red_door.c`, `e_red_door_tiles.c`, `e_init.c`, plus
tous les fichiers déjà migrés et `1CEEC.c`/`2AFCC.c` — aucune nouvelle "conflicting types". **Vrai
build WSL confirmé au premier essai** : `1CEEC.c` garde exactement les 6 mêmes avertissements
baseline, zéro erreur de link, **`✅ us`**. `e_red_door.c` est confirmé octet-exact.

Bo3 a maintenant 12 fichiers migrés au total, tous octet-exact. Reste, non migrables : `e_misc.c`
(avec `EntityOlroxDrool`), `e_life_up.c`. Plus les 5 fonctions zone1 différées.

## e_misc.c : 17 des 21 fonctions étaient partagées (dont `EntityOlroxDrool`, malgré son nom) ; 4 vraiment boss-spécifiques hand-décompilées (2026-07-30) — gcc-clean, UNVERIFIED sur vrai WSL

`e_misc.c` était classé "mixte" à cause du nom `EntityOlroxDrool` (qui semblait forcément lié au
boss Olrox/bo0). Vérification : `src/st/e_misc.h` (header partagé) contient bel et bien
`EntityOlroxDrool`, et `bo0` (Olrox lui-même) inclut ce même header — donc le nom vient juste du
premier boss où la fonction a été identifiée, pas d'un contenu réellement spécifique. `rbo3/e_misc.c`
(Scylla difficile, même boss que bo3) est déjà migré à 100% vers ce header avec seulement 3
déclarations anticipées (`EntityUnkId13`, `EntityExplosionVariants`, `EntityGreyPuff`) — précédent
direct repris tel quel pour bo3.

Restaient 4 fonctions à statut inconnu : `func_us_801AE7E4/8E4/9B0/A78`. Vérification une par une
via la vraie désassemblure (pas de simple stub 1-instruction comme le bug `symbols.us.bobo3.txt`
rencontré sur `create_entity.c` — ce sont des fonctions réelles de 53 à 74 lignes) :

- `func_us_801AE7E4` (E_INTENSE_EXPLOSION, slot 0x06) : confirmé **distinct** du
  `EntityIntenseExplosion` partagé (adresses différentes, 0x801AE7E4 vs 0x801AF7E4, corps
  différent). Hand-décompilé : état d'init (palette/animSet/blendMode selon `params`) puis
  décrément de position + auto-destruction au bout de 0x25 frames.
- `func_us_801AE8E4` : même structure (init + branche animation), hand-décompilée.
- `func_us_801AE9B0` : ressemble structurellement à `MakeEntityFromId` (header partagé) mais **pas
  identique** — l'indexation de la table de dispatch se fait par `entityId` direct, alors que
  `MakeEntityFromId` (et `CreateEntityFromEntity`) font `entityId - 1`. Confirmé par lecture
  attentive de la désassemblure (pas de décrément présent). Gardée comme fonction séparée plutôt que
  aliasée.
- `func_us_801AEA78` : spawner de 6 entités `E_EXPLOSION` (angle/kind aléatoires), utilise les
  champs `ext.destructAnim.unk80/unk84/unk85` — struct déjà définie dans `entity.h`
  (`ET_DestructAnimation`), donc pas de nouveau type à inventer.

Toutes les 4 utilisent des globals déjà connues (`D_us_8018054C` partagé avec `1CEEC.c`/`2AFCC.c`,
nouveau `D_us_80181AB4` boss-spécifique pour l'animation, `D_us_80180424` = `BO3_EntityUpdates`).

**Vérifié (gcc-clean seulement, pas encore de vrai build WSL)** : `gcc -fsyntax-only` propre sur
`e_misc.c` — un seul avertissement à corriger en cours de route (`self->posY -= 0x4000` invalide,
`f32` est une union avec `.val`/`.i.{lo,hi}` — corrigé en `self->posY.val -= 0x4000`), et un piège
évité : ajouter notre propre `extern void CreateEntityFromEntity(...)` après l'`#include` du header
partagé provoque une vraie erreur "conflicting types" contre la déclaration implicite déjà présente
dans `e_misc.h` lui-même (qui appelle cette fonction sans prototype, comme dans toutes les autres
overlays utilisant ce header) — laissé tel quel, pas de prototype ajouté. Aucune nouvelle
"conflicting types" au-delà du baseline connu de `1CEEC.c`/`2AFCC.c`.

**Suite (vrai build WSL, même jour)** : 3 erreurs de link trouvées, même famille "nom générique du
header partagé sans donnée bo3 correspondante" que les `EInit` précédents (`g_EInitCommon` etc.) :
`g_EInitUnkId13`, `g_EInitParticle`, `g_bigRedFireballAnim` (déclarés `extern` dans `e_misc.h` mais
jamais définis côté bo3). Adresses réelles trouvées en lisant les vrais sites d'appel
`InitializeEntity`/`AnimateEntity` dans `EntityUnkId13.s`/`EntityBigRedFireball.s` :
`g_EInitUnkId13` = `D_us_80180570`, `g_bigRedFireballAnim` = `D_us_80181954` (aucun conflit, définis
directement), `g_EInitParticle` = `D_us_8018054C` — **le même bloc déjà utilisé** par
`func_us_801AE7E4`/`8E4` dans ce fichier et par `1CEEC.c`/`2AFCC.c` sous ce nom brut. Fix :
`#define`s ajoutés en haut de `e_misc.c`, **avant** l'`#include` du header partagé (sinon le corps
du header voit les noms génériques avant que l'alias n'existe). `gcc -fsyntax-only` toujours propre
après ce fix, aucune régression.

**Vrai build WSL confirmé (2026-07-30)** : `1CEEC.c` garde exactement les 6 mêmes avertissements
baseline, zéro erreur de link, **`✅ us`**. **`e_misc.c` est confirmé octet-exact.**

Bo3 a maintenant 13 fichiers migrés/résolus, tous octet-exact. Reste, non migrable : `e_life_up.c`.
Plus les 5 fonctions zone1 différées.

## e_life_up.c : `EntityLifeUpSpawn` hand-décompilée via `m2c` + déblocage jumptable (2026-07-30) — gcc-clean, UNVERIFIED sur vrai WSL

Dernier fichier bo3 non migrable (confirmé boss-spécifique dès une session précédente). 587 lignes
d'assembleur réel, table de saut à 7 cas (`jtbl_us_8019CC4C`) bloquant `m2c` ("jump table is not
provided"). Appliqué la méthode de déblocage déjà éprouvée sur `bo1`/zone1 : dans une copie isolée
du `.s`, ajout des labels `.Lus_<adresse>` manquants (5 des 7 cibles n'en avaient pas) + un second
bloc `glabel jtbl_us_8019CC4C` avec des `.word .Lus_<label>` symboliques en fin de fichier — `m2c`
a ensuite produit un premier jet complet du switch à 7 cas.

Nettoyage du jet brut `m2c` (types `?`, noms `unkXX`, `void*`/`void**` génériques) en repassant
chaque offset contre les vraies structs déjà nommées dans le projet : `Entity` (`game.h`), `Primitive`
(0x34 octets, `primitive.h`), et **`ET_LifeUpSpawn`** — déjà défini dans `entity.h` (`prim1`/`prim2`/
`unk84`/`unk86`/`unk88`), confirmant que le nom de la fonction n'était pas trompeur cette fois. Un
détail retail fidèlement préservé (pas "corrigé") : le prim central initialise ses 4 coins X **et**
Y avec la même valeur `posX.i.hi` — vérifié deux fois sur la vraie désassemblure, pas une coquille de
transcription.

**Découverte structurelle** : l'anneau tournant est construit par **paires** de primitives — une
visible (coins d'écran écrits par `RotTransPers4` chaque frame) suivie d'une seconde primitive
"état" dont les champs de coordonnées autrement inutilisés (`x1`/`y1`/`u0` empaquetés en un seul
compteur d'angle, `x2`/`y2` en échelle, `x3`/`y3`/`tpage` en angles de rotation X/Y/Z persistants)
servent de scratch — confirmé en retraçant précisément quel pointeur (`quadPrim` vs `quadPrim->next`)
chaque instruction `lw`/`lh`/`sh` cible réellement, registre par registre, plutôt que de faire
confiance au nommage générique de `m2c`.

3 tables de données boss-spécifiques extraites octet-par-octet de `0.data.s` (jamais nommées avant) :
`D_us_80181C40` (4x `SVECTOR`, coins du quad ±8), `D_us_80181C60` (22x `u16`, table de
prochain-prix/état), `D_us_80181C8C` (1x `SVECTOR`, toujours `{0,0,0}`).

**Vérifié (gcc-clean seulement)** : `gcc -fsyntax-only` propre, zéro nouvel avertissement au-delà du
bruit de base déjà présent dans tous les fichiers du projet (après 3 petits fixs : `long` au lieu de
`s32` pour les pointeurs `RotTransPers`/`RotTransPers4`, externs manquants `MoveEntity`/
`UnkPolyFunc2`, et retrait du `&` sur `InitializeEntity(D_us_80180564)` — `EInit` est un
`typedef u16[5]`, donc le nom du tableau décroît déjà en `u16*`). **Vrai build WSL confirmé du premier coup (2026-07-30)** : `1CEEC.c` garde exactement les 6 mêmes
avertissements baseline, zéro erreur de link, **`✅ us`**. Remarquable pour une fonction de cette
densité (rendu GTE, deux primitives par cellule d'anneau, table de saut) — la reconstruction
`m2c` + nettoyage champ-par-champ contre les vraies structs (`Entity`/`Primitive`/`ET_LifeUpSpawn`)
était donc correcte du premier coup, y compris le détail structurel des paires de primitives.
**`e_life_up.c` est confirmé octet-exact.**

Avec ceci, bo3 a 14 fichiers migrés/résolus, tous octet-exact. Reste : uniquement les 5 fonctions
zone1 différées (`801A5338` en premier). Une fois celles-ci faites, bo3 sera complet.

## 2026-07-30 — bo3 : func_us_801A5338 (tête de chaîne du corps de Scylla)

Première des 5 fonctions zone1 différées de `1CEEC.c`. Analyse manuelle registre par
registre (pas de blocage m2c ici, juste une fonction dense) :

- C'est la tête de la chaîne de corps de Scylla : au step 0, elle alloue 13 quads (bande de
  texture défilante, `u0/u2` et `u1/u3` décrémentés de 14 à chaque quad — effet d'écailles/
  ondulation), puis crée 11 segments de corps (type `0x3B` = `func_us_801A57A4`, déjà
  décompilé) en `self+1..self+11`, et un segment "queue" (type `0x3C` = `func_us_801A5948`,
  encore différé) en `self+12` — tous en tableau contigu `g_Entities_224` (même convention
  que `func_us_801A4FB8/801A516C/801A51E4`, appelées ici même en step 2/3 pour
  animer/relâcher/dessiner la chaîne).
- step 1 : attend soit la proximité du joueur (`params==0`, `GetDistanceToPlayerX`), soit un
  décompte fixe (`params!=0`), avant `SetStep(2)`.
- step 2/3 : appellent les 3 passes de chaîne chaque frame ; step 3 pilote en plus un
  sous-état `step_s` (0 = arme la séquence de mort + drapeau château, 1 = décompte puis arme
  le flag `ext.unk85` d'UN segment à la fois (de la queue vers la tête, au fur et à mesure que
  `unk84` descend), 2 = détruit les 12 entités de la chaîne + soi-même).
- Un contrôle en tout début d'appel fait passer step 2 → 3 via `SetStep(3)` une fois qu'un
  flag partagé (`D_us_80180D48` bit 1) est latché dans `self->flags` bit `0x100`.
- `GetDistanceToPlayerX` n'était pas encore déclarée dans ce fichier (utilisée sans argument
  ailleurs dans le repo, ex. `rbo3.c`) : ajout d'un `extern s32 GetDistanceToPlayerX(void);`.
- Confiance : haute sur le flot de contrôle (vérifié instruction par instruction contre le
  `.s`) ; moyenne sur le sens exact de `D_us_80180D48`/`D_us_80180C66`/`g_CastleFlags[0xC5]`
  (globales déjà partagées ailleurs dans le fichier, sens déduit de l'usage uniquement).

Vérifié en isolation (`gcc -fsyntax-only`) : aucune nouvelle erreur/warning au-delà des 6
erreurs de référence connues du fichier. En attente de confirmation octet-exacte par build
WSL réel.

Reste : 4 fonctions zone1 différées (`func_us_801A0A80`, `func_us_801A2AEC`,
`func_us_801A365C`, `func_us_801A5948`).

## 2026-07-31 — bo3 : func_us_801A0A80 (traqueur de front d'eau, salle inondée)

Deuxième des 5 fonctions différées de `1CEEC.c`. La plus dense des 5 (470 lignes d'asm,
cadre de pile 0x60, 8 registres sauvegardés). Analyse registre par registre, avec deux
pièges découverts en cours de route et corrigés avant même le check gcc :

- `$s3` (self, l'argument de la fonction) et `$fp` (`&g_Entities[0]`, l'entité racine de
  Scylla — même convention déjà documentée sur `func_us_801A4988`) sont utilisés côte à
  côte tout du long ; une première passe avait confondu les deux sur plusieurs accès
  (`self->ext` cache vs champs bruts de la tête de Scylla). Corrigé en re-vérifiant chaque
  `lhu/lh $vX, OFFSET($s3 | $fp)` individuellement contre le `.s`.
- Le cascade de debug (`g_unkGraphicsStruct.D_80097448/4C/50`, affiché ensuite via
  `FntPrint`) avait une première version avec la mauvaise valeur de repli (`gapValue` au
  lieu de `0`) sur la branche de nettoyage partagée (`Lus_801A1080`) — corrigé après
  relecture ligne par ligne de tout l'arbre de branchement.

Résumé fonctionnel : calcule une plage de recherche verticale (`rangeThresh`/`probeOffset`)
selon la forme du joueur (chauve-souris/brume/loup) et l'accroupissement, cherche une
"bande" via `func_us_801A09DC`/`D_us_80180C68`, puis (si `self->step != 0`) fait apparaître
soit des débris d'éclaboussure (type `0x2F`, `func_us_801A1120`) soit une ride d'eau (type
`0x30`, `func_us_801A16E4`) selon le signe de la vélocité de Scylla et l'état de
transformation du joueur. Chaque appel met en cache ses résultats de recherche dans les
champs `ext` de `self` et dans le global `D_us_801B2874` pour comparaison la frame
suivante. `probeDist[1]` (abs `0x8E`) n'est jamais écrit dans cette fonction — lecture de
pile non-initialisée authentique, préservée telle quelle.

Confiance : haute sur le flot de contrôle et les champs (`self` vs `fp` reconfirmés un par
un) ; moyenne sur le sens exact des seuils de jeu et du cascade de debug (fonctionnalité
sans doute jamais visible en jeu final).

Vérifié en isolation (`gcc -fsyntax-only`) : aucune nouvelle erreur/warning au-delà des 6
erreurs de référence connues. En attente de confirmation octet-exacte par build WSL réel —
comme convenu, je n'enchaîne pas sur la suite avant cette confirmation.

Reste : 3 fonctions zone1 différées (`func_us_801A2AEC`, `func_us_801A365C`,
`func_us_801A5948`).

## 2026-07-31 — bo3 : func_us_801A2AEC (cinématique de mort de Scylla)

Troisième des 5 fonctions différées, et la plus grosse (784 lignes d'asm). C'est toute la
séquence de mort/sortie de salle de Scylla (`E_UNK_31`), avec un switch(step) principal
(0-5, 0xFF) et, au step 6, un second niveau de dispatch sur `step_s` via une VRAIE table de
sauts (`jtbl_us_8019CA38`) jamais résolue par splat.

Point clé de méthode : plutôt que deviner les 9 adresses cibles de la table de sauts (comme
pour les jumptables précédentes avec la technique m2c+labels), j'ai directement extrait les
9 mots depuis la ROM retail (`disks/us/BOSS/BO3/BO3.BIN`, offset fichier `0x1CA38` =
adresse VRAM `0x8019CA38` moins la base de segment `0x80180000`) via un script Python. Ça a
donné les 9 adresses réelles sans ambiguïté, bien plus fiable qu'une déduction visuelle sur
un flot de contrôle aussi dense.

Plusieurs erreurs de lecture corrigées avant même le check gcc, en recroisant chaque bloc
avec les adresses réelles :
- Confusion initiale entre `self` (tête de Scylla) et 3 "emplacements virtuels" dans le même
  tableau contigu (`self+2`, `self+15`, `self+28`, tous des champs Entity standards à des
  offsets bruts comme `0xB06`/`0xB28` — recalculés et vérifiés un par un contre
  `entity.h`).
- Une boucle de débris (step 6, sous-état 3) écrivait `params = Random()` dans ma première
  lecture ; le vrai asm montre `params = 1` (constante) et un `Random()` séparé qui, lui,
  conditionne `posY`/`posX` — capturé en relisant l'ordre exact des délais de branchement.
  Les deux autres boucles similaires (sous-états 5 et 7) n'ont PAS cette branche
  conditionnelle, contrairement à ma première hypothèse par analogie.
- Un octet à `self+0x85` (ext+9) n'appartient à aucun champ Entity nommé — ajouté comme champ
  `ext->unk85` local à cette fonction plutôt que mal-nommé.

Vérifié en isolation (`gcc -fsyntax-only`) : aucune nouvelle erreur au-delà des 6 connues (le
seul changement de compte concerne `AnimateEntity`, qui passe de 4 à 5 occurrences du même
conflit déjà toléré partout ailleurs dans ce fichier — pas une nouvelle catégorie de
problème). En attente de confirmation octet-exacte par build WSL réel.

**Confirmé octet-exact par build WSL réel** (`✅ us`, mêmes 6 conflits de référence,
`AnimateEntity` chaîné sur 3 redéclarations comme attendu).

Reste : 2 fonctions zone1 différées (`func_us_801A365C`, `func_us_801A5948`).

## 2026-07-31 — bo3 : func_us_801A365C (épilogue post-cinématique de Scylla)

Quatrième des 5 fonctions différées. C'est la suite directe de `func_us_801A2AEC` sur la
même entité racine : une fois l'explosion terminée, cette fonction gère le "coulage dans
l'eau" final — switch(step) 0-6, avec deux niveaux de dispatch supplémentaires sur
`step_s` via deux VRAIES tables de sauts jamais résolues par splat (`jtbl_us_8019CA60`,
7 entrées, et `jtbl_us_8019CA80`, 5 entrées).

Mêmes techniques que pour `func_us_801A2AEC` :
- Les 12 adresses cibles des deux tables ont été extraites directement de la ROM retail
  (`disks/us/BOSS/BO3/BO3.BIN`, offsets fichier `0x1CA60` et `0x1CA80`) via le même script
  Python — toujours plus fiable que la déduction depuis le flot de contrôle.
- Plusieurs blocs de code sont **partagés au niveau assembleur** entre des `step`
  différents (ex. le step 3 sous-état 0/1 et le step 5 sous-état 0/1 sautent vers les
  mêmes adresses avec des constantes différentes préchargées avant le saut — fusion de
  queue faite par le compilateur). Reproduit en écrivant le même code C dans chaque
  `case`, en comptant sur le compilateur pour refaire la même fusion (comme déjà observé
  avec la queue partagée `SetStep(2); return;` des fonctions précédentes).
- Step 4 (sous-états 0-3) fait converger 3 sous-états vers une rafale partagée de 8
  entités de type `0x38` — le sous-état 3 (transition finale) n'atteint jamais cette
  rafale, contrairement aux sous-états 0/1/2.
- Champ `self->unk50` (mot compacté pose/poseTimer) réutilisé ici exactement comme dans
  `func_us_801A2AEC`, même sémantique.

Confiance : haute sur le flot de contrôle (chaque frontière step/sous-état recroisée avec
le `.s`, les deux tables résolues depuis la ROM) ; moyenne sur le sens exact des bits
`D_us_80180D48` (partagés avec `func_us_801A2AEC`, déduits par usage croisé uniquement).

Vérifié en isolation (`gcc -fsyntax-only`) : aucune nouvelle erreur au-delà des 6 connues
(`AnimateEntity` passe à 5 occurrences, même conflit toléré). En attente de confirmation
octet-exacte par build WSL réel — je n'enchaîne pas sur la dernière fonction avant cette
confirmation.

**Confirmé octet-exact par build WSL réel** (`✅ us`, mêmes 6 conflits de référence,
`AnimateEntity` chaîné sur 3 redéclarations comme attendu).

Reste : 1 fonction zone1 différée (`func_us_801A5948`) — la dernière de bo3.

## 2026-07-31 — bo3 : func_us_801A5948 (attaque de tête de Scylla, dernière fonction différée)

Cinquième et dernière des fonctions différées de `1CEEC.c`. C'est l'entité "queue" du
serpent de corps (`self`, type `0x3C`, créée par `func_us_801A5338` à `self+12` de sa
propre racine) : l'attaque de tête/projectile de Scylla, lancée depuis le bout de la
chaîne. Dispatch principal `switch(self->step)` (0-6) via une VRAIE table de sauts jamais
résolue par splat (`jtbl_us_8019CAC8`, 7 entrées, même technique d'extraction ROM que les
2 fonctions précédentes) ; sous-dispatch `step_s` (0/1) au step 6.

Découverte clé : `self` n'a pas de pointeur explicite vers sa racine ni vers le segment de
corps précédent — les deux sont atteints par de l'arithmétique de pointeur brute sur
`self` avec des offsets négatifs (`self - 0x8D0`/`self - 0x84A` pour la racine et son champ
`ext.unk86`, `self - 1` via `self - 0xB8`/`-0xBC` pour le segment précédent). Confirmé
précisément : `0x8D0 == 12 * sizeof(Entity)` exactement, cohérent avec le commentaire déjà
présent sur `func_us_801A5338` ("type 0x3C à `self+12`") — donc `root = self - 12`,
`prev = self - 1`, pas de suppositions.

Autres points notables :
- Tout le flux (y compris les sorties anticipées "encore en attente"/"encore en train
  d'animer") converge vers une queue commune qui recalcule `self->rotate` par rapport au
  segment précédent (`prev`) à chaque frame — modélisé en C comme du code après le
  `switch` principal plutôt qu'un retour anticipé, puisque `self->rotate` doit être
  recalculé sur TOUS les chemins.
- Encore un partage de code au niveau assembleur entre deux `case` (step 1 saute dans le
  MILIEU du corps du step 2 une fois son attente terminée) — même idiome que
  `func_us_801A365C`, reproduit en écrivant le même code C dans les deux `case` et en
  laissant le compilateur refaire la fusion.
- Le calcul de cible du step 2 mélange le Y du joueur (`PLAYER_posY_val`) avec le X de la
  tête de Scylla (`g_Entities[0].posX.val`) — préservé tel quel plutôt que "corrigé" vers
  une cible cohérente, cette asymétrie est bien présente dans l'asm réel.
- Deux vérifications de distance à des échelles différentes (step 5 : distance en pixels
  entiers via `>> 16` ; step 6 sous-état 1 : distance en unités fixed-point brutes, un
  seuil bien plus strict) — gardées telles quelles sans les harmoniser.

Confiance : haute sur le flot de contrôle et l'arithmétique racine/précédent (vérifiée
arithmétiquement, pas devinée) ; moyenne sur le sens exact de la cible mixte du step 2 et
des deux échelles de distance différentes (comportement préservé, intention non garantie).

Vérifié en isolation (`gcc -fsyntax-only`) : aucune nouvelle erreur au-delà des 6 connues
(`AnimateEntity` passe à 6 occurrences, même conflit toléré ; aucun avertissement à
l'intérieur de la fonction elle-même). En attente de confirmation octet-exacte par build
WSL réel.

**Confirmé octet-exact par build WSL réel** (`✅ us`, mêmes 6 conflits de référence).

**Les 5 fonctions différées de zone1 sont maintenant décompilées ET confirmées.**
`1CEEC.c` (zone1 de bo3, 54 fonctions) n'a plus aucun `INCLUDE_ASM` restant — **bo3 zone1
est terminée à 100%, prouvée octet-exacte, pas juste plausible.**

## 2026-07-31 — bo1 (Granfaloon) : début de la migration moteur, 1er checkpoint — gcc-clean, NON VÉRIFIÉ sur build WSL réel

bo3 étant terminé, passage à bo1. Contrairement à bo3, le code spécifique au boss
(`20F60.c`, 18 fonctions) était déjà fait depuis longtemps — ce qui restait, c'est
exactement le travail de "migration" que bo3 a demandé : brancher les fichiers moteur
partagés vers les headers `src/st/*.h` déjà établis.

Différence notable avec bo3 : le blob de données non identifié en tête de fichier fait
**133 Ko** ici (contre 1 Ko pour bo3), et bo1 a un fichier `e_collect.c` (16 fonctions,
~2800 lignes d'asm) qui n'existe dans AUCUN autre boss déjà traité — pas une migration,
du code moteur jamais décompilé nulle part dans le projet. Décision : traiter ce fichier
(et les autres vrais orphelins : `create_entity.c`, `e_misc.c`, `e_red_door.c`,
`e_life_up.c`) dans un checkpoint séparé, et migrer d'abord tout ce qui est sûr.

**Table de dispatch localisée directement depuis la ROM** (`disks/us/BOSS/BO1/BO1.BIN`),
même technique que bo3 : le header `Overlay` (`Update`/`HitDetection`/
`UpdateRoomPosition`/`InitRoomEntities`/...) confirmé mot pour mot contre les adresses
réelles de `config/symbols.us.bobo1.txt` à l'offset 0 du fichier ; `MakeEntityFromId.s`
(dans `e_misc.c`, encore orpheline) référence la table de dispatch par
`D_us_801809F4` — extraite (38 entrées, offset fichier `0x9F4`) et toutes identifiées :
soit vers les 18 fonctions déjà connues de `20F60.c`, soit vers des noms partagés déjà
attribués par splat dans `symbols.us.bobo1.txt` (`EntityExplosion`, `EntityPrizeDrop`,
`EntitySoulStealOrb`, etc.), soit vers `func_us_801A0CDC`/`func_us_801A0DA4`
(`e_room_bg.c`/`e_lock_camera.c`, encore orphelines à ce stade).

**`e_room_bg.c` et `e_lock_camera.c` confirmés 100% partagés** avant migration (comme sur
bo3) : structure de `func_us_801A0CDC` recroisée champ par champ contre
`src/st/e_room_bg.h` (stride `ObjInit2` de 0x14 octets, même ordre de champs exact) ;
`func_us_801A0DA4` recroisée contre `src/st/entity_lock_camera.h` — et pour celui-ci,
les 3 tables que le header code en dur (`entityLockCameraHitbox`/`Data`/
`TilemapProps`) correspondent **octet pour octet** aux vraies données de la ROM
(`D_us_80180B9C`/`BA4`/`BAC`), donc aucune donnée par-boss à dupliquer, juste
`g_EInitLockCamera` à fournir.

**5 valeurs `EInit` trouvées** en lisant les vrais sites d'appel `InitializeEntity` dans
les `.s` encore orphelins (`func_us_801A0CDC`, `func_us_801A0DA4`, `EntityDamageDisplay`,
`EntitySoulStealOrb`/`EntityEnemyBlood`, `EntityStageNamePopup`) : `g_EInitCommon` =
`{0,0,0,0,3}`, `g_EInitLockCamera` = `{0,0,0,0,1}`, `EInitObtainable` = `{3,0,0,0,1}`,
`g_EInitInteractable` = `{0,0,0,0,5}`, `g_EInitDamageNum` = `{0,0,0,0,3}` — **toutes
identiques aux valeurs déjà confirmées sur bo3** pour ces mêmes constantes (convention
apparemment universelle au moteur, pas propre à un boss).

**Vérification systématique des conflits de nom avant d'écrire `bo1.h`** (leçon de bo3,
"toujours vérifier si un fichier encore orphelin référence déjà cette adresse par son nom
brut") : `D_us_80180AE0` (g_EInitCommon), `D_us_80180A98` (EInitObtainable),
`D_us_80180ABC` (g_EInitInteractable) et `D_us_801809F4` (table de dispatch) sont bien
référencés par leur nom brut dans les fichiers encore orphelins (`e_misc.c`/
`e_red_door.c`) — gardés tels quels, aliasés dans `bo1.h`, même schéma que bo3.
`g_EInitLockCamera`/`g_EInitDamageNum`/la table `BackgroundBlockInit` n'ont aucune
référence brute ailleurs — déclarés directement.

Créé `bo1.h` + `e_init.c` (table de dispatch 38 entrées + les 5 `EInit` + une seule
entrée confirmée pour `BackgroundBlockInit`, la deuxième entrée candidate dans la ROM
ayant des valeurs répétitives suspectes qui ne ressemblent pas à de vraies données
`ObjInit2` — probablement que `params` vaut toujours 0 pour ce boss, non confirmé contre
un vrai parcours de layout de salle). Migré les 10 fichiers sûrs : `st_common.c`,
`prim_helpers.c`, `st_update.c`, `popup.c`, `e_room_bg.c`, `e_lock_camera.c`,
`e_particles.c`, `e_room_fg.c`, `e_stage_name.c`, `st_collision.c`. Chirurgie de yaml
splat identique à bo3 : `[0x9F4, .data, e_init]` carvé dans le blob non identifié.

Vérifié en isolation (`gcc -fsyntax-only`) sur les 12 fichiers touchés (10 migrés +
`bo1.h`/`e_init.c`) plus `20F60.c` : **zéro erreur, zéro avertissement nouveau** — seuls
les avertissements de déclaration implicite déjà inhérents aux headers partagés eux-mêmes
(`AnimateEntity` dans `e_room_bg.h`, `GetSideToPlayer` dans `entity_lock_camera.h`,
`PlaySfxPositional` dans `st_common.h`), rien de spécifique à bo1. Pas de conflit de type
« baseline » à la bo3 — `20F60.c` compile propre tout seul. `splat split` non relancé
dans ce sandbox (config/disques absents de la copie isolée utilisée pour le lint) ; à
confirmer via un vrai build WSL.

En attente de confirmation octet-exacte par build WSL réel avant d'attaquer le second
checkpoint (`create_entity.c`, `e_misc.c`, `e_red_door.c`, `e_life_up.c`, puis
`e_collect.c` à part).

**Premier retour WSL : 4 erreurs de link `multiple definition`** (`D_us_80180A98`,
`D_us_80180ABC`, `D_us_80180AE0`, `D_us_80180B84`) — même famille de bug que sur bo3
("garder le nom brut" ne suffit pas seul : tant que ces adresses restent dans le blob
`data` générique non subdivisé, `splat` les auto-étiquette lui-même dans le blob ET notre
`e_init.c` les définit, d'où le conflit). Corrigé en subdivisant explicitement le yaml
`splat.us.bobo1.yaml` — un sous-segment `.data` nommé par adresse pour chacune des 5
constantes `EInit` + les données `animFrames`/table `BackgroundBlockInit`, même technique
que `splat.us.bobo3.yaml` (qui carve individuellement chaque petite donnée identifiée
plutôt que de laisser un seul gros blob générique).

**Bug trouvé et corrigé au passage** : `D_us_80180B84` (les vrais octets `animFrames`)
avait été mal transcrit dans la première passe — j'avais recopié par erreur les champs de
la TABLE `BackgroundBlockInit` elle-même (`{0x06, 0x01, 0xFA, 0x01, 0x00, 0x00}`, soit
`animSet`/`zPriority`) au lieu de relire les vrais octets à l'adresse `D_us_80180B84`.
Ré-extrait précisément depuis la ROM : `{0x40, 0x01, 0xFF, 0x00}` (une paire
frame/durée terminée par `0xFF`, même encodage que les tables `animFrames` de bo3).
Leçon : toujours re-vérifier par script à l'adresse EXACTE plutôt que de réutiliser une
valeur déjà lue dans un contexte voisin.

Re-vérifié en isolation (`gcc -fsyntax-only`) : toujours propre.

**Confirmé octet-exact par build WSL réel** (`✅ us`, mêmes 6 conflits de référence de
bo3, rien de nouveau côté bo1). **Premier checkpoint bo1 terminé** : `st_common.c`,
`prim_helpers.c`, `st_update.c`, `popup.c`, `e_room_bg.c`, `e_lock_camera.c`,
`e_particles.c`, `e_room_fg.c`, `e_stage_name.c`, `st_collision.c` migrés et
byte-exact ; `bo1.h`/`e_init.c` en place.

Reste sur bo1 : `create_entity.c`, `e_misc.c`, `e_red_door.c`, `e_life_up.c` (vrais
orphelins à trier/décompiler à la main, même travail que bo3), et `e_collect.c` à part
(16 fonctions, ~2800 lignes, jamais fait sur aucun boss).

## 2026-07-31 — bo1 : 2ème checkpoint, classification + migration des 4 fichiers restants

Classification faite par vérification structurelle réelle (pas juste par nom de stub),
comme demandé — comparaison instruction par instruction avec les headers partagés et,
pour `create_entity.c`, corroboration croisée avec `bo0`/`bo4` (bosses déjà finalisés en
amont utilisant le même header sans aucune modif).

Résultat, **les 4 fichiers sont en fait 100% partagés** — aucun vrai orphelin cette fois :

- `create_entity.c` : les 15 fonctions matchent `src/st/create_entity.h` mot pour mot.
  Vérifié en détail `InitRoomEntities` et `CreateEntityFromLayout` contre l'asm réel.
- `e_life_up.c` : `EntityLifeUpSpawn` matche `src/st/e_life_up.h` (InitializeEntity sur
  `g_EInitInteractable`=`D_us_80180ABC`, `g_api.func_800EDB58(PRIM_GT4, 385)` identiques).
  **Différent de bo3** où cette même fonction était 100% custom — bonne chose de ne pas
  avoir supposé pareil sans vérifier.
- `e_misc.c` : les 18 stubs sont TOUS dans `src/st/e_misc.h`, y compris
  `UnkRecursivePrimFunc1`/`UnkRecursivePrimFunc2` que j'avais d'abord loupés (mon premier
  grep de recherche de signature ne matchait pas `Primitive* Func(...)`, seulement
  `void`/`s32`/etc. — corrigé). 3 adresses locales à bo1 nécessaires (juste `extern`,
  pas de contenu — même piège d'erreur de lien "multiple definition" que le 1er
  checkpoint si on les redéfinit) : `g_EInitUnkId13`=`D_us_80180AC8`,
  `g_EInitParticle`=`D_us_80180AA4`, `g_bigRedFireballAnim`=`D_us_80181950`.
- `e_red_door.c` : `EntityIsNearPlayer` partagé (attendu). **`func_us_801A7D80` s'est
  révélé être `EntityRedDoor` elle-même**, pas un orphelin — découverte faite en
  comparant la table `g_eRedDoorUV` hardcodée dans `e_red_door.h` (24 octets) contre les
  vrais octets ROM à `D_us_8018142C` : match exact. Nouveau fichier
  `e_red_door_tiles.c` créé pour `RedDoorTiles[2][8]` (valeurs réelles extraites de la
  ROM à `D_us_80180B64`, offset `config/splat.us.bobo1.yaml` carvé en conséquence).
  Table de dispatch (`e_init.c`, slot `0x05`) mise à jour : `func_us_801A7D80` →
  `OVL_EXPORT(EntityRedDoor)`.

Les 6 fichiers touchés (`create_entity.c`, `e_life_up.c`, `e_misc.c`, `e_red_door.c`,
`e_red_door_tiles.c`, `e_init.c`) + `bo1.h` compilent proprement en isolation
(`gcc -fsyntax-only`, zéro nouvelle erreur, seulement les avertissements habituels de
déclaration implicite déjà vus sur bo3/le 1er checkpoint bo1).

**Reste sur bo1 après ce checkpoint : seulement `e_collect.c`** (16 fonctions, ~2800
lignes, travail neuf jamais fait sur aucun boss — pas une migration).

### Correction post-WSL : tables de room layout manquantes

Premier essai WSL : lien échoué, `undefined reference to BO1_pStObjLayoutHorizontal` /
`BO1_pStObjLayoutVertical` — oubli de définir les tables `OBJ_LAYOUT_HORIZONTAL`/
`VERTICAL` que `create_entity.h` attend (même chose que bo3 avait dû faire). Corrigé :

- Repéré les adresses réelles via `InitRoomEntities` (déjà identifiées lors de la
  vérification structurelle initiale) : `D_us_80180850` (horizontal) et
  `D_us_80180924` (vertical).
- **Piège évité** : une première extraction naïve à 53 entrées pour les deux tables
  aurait fait lire, pour la 53ème entrée verticale, le premier slot de la table de
  dispatch elle-même (`D_us_801809F4`, déjà confirmée) plutôt qu'une vraie donnée de
  room. Vérifié directement dans les octets ROM : horizontale a bien 53 entrées
  (0x850-0x924), verticale seulement 52 (0x924-0x9F4, s'arrête pile où la table de
  dispatch commence).
- 12 blobs `LayoutEntity` uniques extraits par scan de terminateur (même convention
  que bo3 : header `{0xFFFE,0xFFFE,0,0,0}`, queue `{0xFFFF,0xFFFF,0,0,0}`), tous
  contigus (0x1BCC-0x1D84).
- `config/splat.us.bobo1.yaml` : carvé `[0x850]`/`[0x924]` (tables de pointeurs) et
  `[0x1BCC]`/`[0x1D84]` (contenu des blobs).
- `bo1.h` : ajouté `BO1_pStObjLayoutHorizontal`/`Vertical` (alias raw, même pattern
  que `BO1_EntityUpdates`).

Re-vérifié en isolation (`gcc -fsyntax-only`) : toujours propre, `create_entity.c`
compile sans erreur.

**Confirmé octet-exact par build WSL réel** (`✅ us`, mêmes 6 conflits de référence de
bo3 que d'habitude, rien de nouveau, aucun échec de lien cette fois). **2ème checkpoint
bo1 terminé** : `create_entity.c`, `e_life_up.c`, `e_misc.c`, `e_red_door.c`,
`e_red_door_tiles.c` migrés/créés, `e_init.c`/`bo1.h` mis à jour (tables de room layout
+ dispatch `EntityRedDoor`), tout byte-exact.

**Reste sur bo1 : seulement `e_collect.c`** (16 fonctions, ~2800 lignes, travail neuf
jamais fait sur aucun boss — pas une migration).

## 2026-07-31 — bo1 : 3ème checkpoint, e_collect.c

**Correction de la mémoire projet en cours de route** : la note "e_collect.c = travail
neuf jamais fait sur aucun boss" était fausse. `src/st/e_collect.h` existe déjà et
`bo0`/`bo4` (bosses déjà finalisés en amont) migrent leur `e_collect.c` avec le même
pattern trivial que les autres fichiers partagés. Vérifié les 16 stubs de bo1 contre ce
header : les 16 y sont, y compris `CollectSubweapon` (dans `collect_subweapon.h`, inclus
depuis `e_collect.h`) et `BlitChar` (dans `blit_char.h`, idem).

Deux données propres à bo1 nécessaires :
- `HEART_DROP_CASTLE_FLAG = 0` — confirmé en comparant `EntityHeartDrop`'s asm réel
  (aucune instruction d'addition visible avant l'indexation de `g_CastleFlags`), même
  valeur que bo0/bo4.
- `BO1_PrizeDrops[16]` (nouveau fichier `d_prize_drops.c`) — **contrairement à bo4**
  (qui a un tableau vide, `ZERO_LEN`, faute de contenu réel), le ROM de bo1 contient
  bien 16 vraies entrées à `D_us_80181020` (`{0,0,0,0,1,0,1,0,1,1,1,1,1,1,1,0x101}`) :
  Granfaloon a un vrai contenu de pièces cachées dans sa salle. Frontière confirmée
  directement dans `asm/us/boss/bo1/data/0.data.s`, où splat avait déjà isolé
  `D_us_80181020` comme un symbole séparé se terminant pile avant `D_us_80181040`
  (une autre table, sans rapport).

`config/splat.us.bobo1.yaml` : carvé `[0x1020, .data, d_prize_drops]` / `[0x1040, data]`.

Compile proprement en isolation (`gcc -fsyntax-only`), avertissements de déclaration
implicite habituels seulement (`MoveEntity`, `BottomCornerText`, `SetStep`,
`AnimateEntity`, `CheckFieldCollision`) — aucune nouvelle erreur, aucun symbole
manquant (confirme aussi que `EInitBreakable`, utilisé ailleurs dans `e_collect.h`,
est bien exclu par `#if` pour les overlays boss, pas besoin de la définir).

**Si ce checkpoint est confirmé par le build WSL réel, bo1 sera 100% terminé.**

### Correction : ordre des segments yaml

Premier essai WSL : splat a refusé de tourner (`segments out of order - (1040 starts at
0x1040, but next segment starts at 0x850)`) — j'avais inséré le carve `[0x1020]`/`[0x1040]`
tout en haut du fichier (juste après `[0x0, data]`), alors que 0x1020 est numériquement
après 0x850/0x924/etc. splat exige un ordre d'adresses strictement croissant. Déplacé le
carve juste après `[0xB9C, data]` (sa vraie position dans l'ordre des adresses). Ordre
vérifié programmatiquement sur les 41 entrées du fichier — strictement croissant.

En attente de confirmation (2ème essai).

### Correction : `g_EInitParticle` non défini (lien échoué)

2ème essai WSL : splat a tourné, mais le lien final a échoué —
`undefined reference to g_EInitParticle` (2 sites d'appel dans `e_collect.c.o`).

Cause : `e_misc.h` ET `e_collect.h` référencent tous les deux `g_EInitParticle` par
son nom littéral réel (pas via `OVL_EXPORT`), donc UN SEUL symbole partagé doit exister
dans tout le programme. Le "correctif" du checkpoint précédent (`extern
D_us_80180AA4[]` + `#define g_EInitParticle D_us_80180AA4` dans `e_misc.c`) était en
fait une erreur : ça renommait la référence de `e_misc.h` vers un symbole que
`e_collect.c` ne fournit jamais. `e_misc.c` compile seul sans souci (pas d'erreur gcc
possible ici, c'est purement un problème de lien), mais le lien final avec
`e_collect.c` échoue puisque personne ne définit réellement `g_EInitParticle`.

Vérifié la convention réelle utilisée par les autres stages déjà finalisées
(`src/st/are/e_init.c` notamment) : `g_EInitParticle` (et `g_EInitUnkId13`) sont
définis pour de vrai, sous leur nom littéral, directement dans `e_init.c` — pas
aliasés à une adresse brute locale à `e_misc.c`. Corrigé :

- `e_init.c` : ajouté `EInit g_EInitParticle = {3,0,0,0,2};` (valeur confirmée contre
  `D_us_80180AA4`).
- `e_misc.c` : retiré l'alias `D_us_80180AA4`/`g_EInitParticle` et l'alias
  `D_us_80181950`/`g_bigRedFireballAnim` (`g_bigRedFireballAnim` est en fait défini
  EN DUR dans `e_collect.h` lui-même, avec un contenu confirmé octet pour octet contre
  `D_us_80181950` — donc `e_misc.c` n'a besoin que de l'`extern` déjà présent dans
  `e_misc.h`, rien de plus). Gardé l'alias `g_EInitUnkId13`/`D_us_80180AC8` tel quel :
  vérifié qu'il n'est référencé nulle part ailleurs par son nom littéral, donc pas de
  conflit.
- `config/splat.us.bobo1.yaml` : carvé `[0xAA4, .data, e_init]` (g_EInitParticle) et
  `[0x1950, .data, e_collect]` (g_bigRedFireballAnim, 26 octets). Ordre des adresses
  re-vérifié programmatiquement.

Compile proprement en isolation (rien de plus à en attendre ici — ce genre d'erreur
de lien n'est visible que par un vrai build WSL, `gcc -fsyntax-only` ne la voit pas).

En attente de confirmation (3ème essai).

### Correction : `D_us_80180AA4` non défini (lien échoué, 4ème essai)

3ème essai WSL : le build est allé beaucoup plus loin (2220/2569) mais a échoué au
lien final — `undefined reference to D_us_80180AA4` (2 sites d'appel dans
`20F60.c.o`, fonctions `LM28` et `LM1646`).

Cause : `20F60.c` (code spécifique à Granfaloon, déjà décompilé à la main lors d'un
checkpoint précédent, n'inclut pas `bo1.h`) contient sa propre déclaration locale
`extern u16 D_us_80180AA4[];` et appelle `InitializeEntity(D_us_80180AA4)`
directement, par nom brut. Le correctif précédent avait supprimé cette exposition en
définissant la donnée uniquement sous le nom littéral `g_EInitParticle` dans
`e_init.c` — ce qui a réglé le lien pour `e_misc.c`/`e_collect.c` mais cassé
`20F60.c`, qui n'a jamais été touché par ce checkpoint et attend toujours le nom brut.

Les deux noms (`g_EInitParticle` et `D_us_80180AA4`) désignent exactement la même
donnée ; il faut UN seul symbole réel exposé sous les deux noms. Corrigé :

- `e_init.c` : la définition réelle est maintenant sous le nom brut —
  `EInit D_us_80180AA4 = {3,0,0,0,2};` (au lieu de `g_EInitParticle`).
- `bo1.h` : ajouté `extern EInit D_us_80180AA4; #define g_EInitParticle
  D_us_80180AA4` — même pattern déjà utilisé pour `g_EInitCommon`/`g_EInitInteractable`
  au-dessus. Tout fichier qui inclut `bo1.h` (donc `e_init.c`, `e_misc.c`,
  `e_collect.c`) voit `g_EInitParticle` réécrit vers `D_us_80180AA4` par le
  préprocesseur ; `20F60.c` n'inclut pas `bo1.h` et continue de référencer
  `D_us_80180AA4` directement — les deux résolvent maintenant vers le même symbole
  réel unique.
- Aucun changement dans `20F60.c` lui-même (fichier figé, déjà confirmé WSL lors d'un
  checkpoint antérieur).

Vérifié en isolation : `e_init.c`, `e_misc.c`, `e_collect.c` et `20F60.c` compilent
tous les quatre sans erreur (`gcc -fsyntax-only`, aucune ligne `error:`).

**4ème essai WSL : ✅ confirmé.** Build complet (`2569/2569`, `✅ us`). Les seuls
warnings restants (`1CEEC.c` conflicting types, `692E8.c` `$at`) sont le bruit de
fond déjà connu et sans rapport (bo3/dra, préexistant). e_collect.c est donc migré
et byte-exact.

**bo1 est maintenant 100% complet et confirmé WSL byte-exact**, y compris
e_collect.c (qui n'était finalement pas du "travail neuf" contrairement à ce qui
avait été noté précédemment — juste une migration standard vers `src/st/e_collect.h`
comme les autres).

## bo2 (Minotaur/Werewolf) — 1er checkpoint (2026-07-31)

Passage à bo2/bo5/bo7 (aucun n'a encore ni son code boss-spécifique ni la migration
engine). État initial : chaque boss a le même jeu de ~16 fichiers engine partagés
(create_entity, e_misc, e_collect, etc.) que bo1/bo3, plus 1-2 fichiers de code
boss-spécifique jamais commencé (bo2 : `22610.c` 24 fonctions + `337D0.c` 8
fonctions ; bo5 : `1FD30.c` 37 fonctions ; bo7 : `13258.c` 14 fonctions).

Contrairement à bo1, `config/symbols.us.bobo2.txt` contenait déjà la plupart des
adresses réelles (travail de scan d'une session antérieure), ce qui a beaucoup
simplifié l'archéologie. Créé `bo2.h` + `e_init.c` (même schéma que bo1/bo3) :

- Table de dispatch des entités (`BO2_EntityUpdates`, 41 entrées, VRAM 0x80180414)
  confirmée via la disassembly réelle de `CreateEntityFromLayout.s`
  (`D_us_80180410` == `PfnEntityUpdates - 4`) et croisée entrée par entrée avec
  `symbols.us.bobo2.txt` (entrée 0 == `EntityBreakable` == 0x801A24DC).
- `pStObjLayoutHorizontal`/`Vertical` (create_entity.h), confirmées via
  `InitRoomEntities.s` — **53 entrées chacune cette fois, symétrique** (contexte
  bo1 avait 53/52 asymétrique). 10 blobs `LayoutEntity` uniques transcrits
  octet pour octet (bien moins de variantes de room-layout que bo1).
- 8 constantes EInit localisées et confirmées par recoupement avec les vrais
  sites d'appel `InitializeEntity` dans l'asm figé de chaque fichier.
- **Piège détecté AVANT le premier essai WSL cette fois** (pas après, contrairement
  à bo1) : grep systématique de `asm/us/boss/bo2/nonmatchings/{22610,337D0}` (le
  code boss-spécifique encore non migré) pour CHAQUE adresse EInit candidate,
  avant d'écrire `bo2.h`/`e_init.c`. Résultat : `g_EInitParticle`,
  `g_EInitInteractable` et `g_EInitCommon` sont référencés par nom brut
  directement depuis `22610.c`/`337D0.c` (fichiers gelés, pas touchés ce
  checkpoint). Corrigé directement dans `e_init.c`/`bo2.h` avec le même schéma
  que la correction bo1 (définition sous le nom brut + alias `#define` dans
  bo2.h) — **zéro aller-retour WSL gaspillé** sur cette classe d'erreur cette
  fois.
- `e_breakable.c` (nouveau pour bo2, ni bo1 ni bo3 ne l'avaient) : 5 tables
  statiques + 2 tableaux d'animation transcrits octet pour octet depuis la ROM,
  confirmés via la disassembly réelle d'`EntityBreakable`.
- `e_room_bg.c` (`EntityBackgroundBlock`) : `BackgroundBlockInit` (2 entrées
  réelles, confirmées comme bo1 en vérifiant qu'une 3ème entrée hypothétique
  reproduit les octets de `RedDoorTiles`) + 2 tableaux `animFrames`.
- `e_red_door.c` : `func_us_801A9E50` confirmé == `EntityRedDoor` (même
  technique que bo1 : `InitializeEntity(g_EInitCommon)` + table `RedDoorTiles`
  correspondant octet pour octet).
- **`e_misc.c` n'est PAS 100% partagé cette fois** (différent de bo1/bo3) :
  contient 4 fonctions boss-spécifiques réelles imbriquées (dont l'entrée 6 de
  la table de dispatch, `func_us_801AF744`), pas de simples artefacts de
  jumptable (vérifié : 53-74 lignes d'asm chacune, pas les stubs de 6 lignes
  qui se sont révélés être des artefacts jumptable dans `create_entity.c`).
  **Reporté à un prochain checkpoint** — nécessite une vraie décompilation, pas
  juste un swap d'en-tête. `create_entity.c` migré normalement malgré ses 8
  stubs `func_us_` non nommés (confirmés comme artefacts jumptable via leur
  taille, 6 lignes chacun).

Migré ce checkpoint : `create_entity`, `e_life_up`, `e_lock_camera`,
`e_particles`, `e_red_door`, `e_room_bg`, `e_room_fg`, `e_stage_name`, `popup`,
`prim_helpers`, `st_collision`, `st_common`, `st_update`, `e_collect`,
`e_breakable` (15 fichiers). `e_misc.c` non touché (voir ci-dessus).

Vérifié en isolation (`gcc -fsyntax-only`) : les 17 fichiers (bo2.h, e_init.c +
15 migrés) compilent sans erreur après 2 corrections mineures (déclarations
anticipées manquantes dans e_init.c pour `EntityUnkId13`/
`EntityExplosionVariants`/`EntityGreyPuff`/`EntityLifeUpSpawn` ; double-inclusion
de `animate_entity.h` dans `st_common.c`, déjà inclus par `st_common.h`
lui-même).

En attente de confirmation WSL (1er essai).

### Correction : lien échoué (1er essai bo2)

1er essai WSL : le build a bien plus loin progressé (2418/2581) mais a échoué au
lien de `bobo2.elf` avec ~15 `undefined reference`. Deux causes distinctes, aucune
détectable par `gcc -fsyntax-only` (comme toujours pour ce genre d'erreur) :

**1. `create_entity.c` n'était PAS un migration sûre — contrairement à bo1/bo3.**
Les 8 stubs `func_us_` non nommés (`func_us_801A9808`, `...9854`, `...98AC`,
`...99A8`, `...9ABC`, `...9C34`, `...9CE8`, `...9D5C`) avaient été jugés
« artefacts de jumptable inoffensifs » sur la seule foi de leur petite taille
(6 lignes chacun). Erreur : relecture du contenu réel a montré que
`UpdateRoomPosition` lui-même fait `jal func_us_801A99A8` en interne, et
`22610.c`/`337D0.c`/`e_misc.c` (tous encore figés) appellent
`func_us_801A9CE8`/`func_us_801A9D5C` des dizaines de fois au total — ce sont de
vraies adresses réelles, pas des artefacts. Migrer `create_entity.c` vers l'en-tête
partagé les a fait disparaître du lien (le compilateur ne les régénère pas sous
ces noms). **Corrigé en annulant la migration de `create_entity.c`** (retour aux
stubs `INCLUDE_ASM` d'origine) — reporté à un futur checkpoint, comme `e_misc.c`.
**Leçon** : la taille d'un stub non nommé ne suffit pas à conclure « artefact » ;
il faut lire le contenu réel et vérifier si d'autres fichiers l'appellent via
`jal`.

**2. Trois adresses supplémentaires référencées par nom brut, ratées au premier
passage** (le grep initial ne couvrait que 22610.c/337D0.c, pas le blob de
données brutes `0.data.s` ni le contenu figé d'`e_misc.c` lui-même) :
- `D_us_8018026C`/`D_us_80180340` (tables pStObjLayoutHorizontal/Vertical) —
  référencées directement par le blob Overlay struct encore non identifié.
  Redéfinies sous leur nom brut directement dans `e_init.c` (plus besoin d'alias
  friendly puisque `create_entity.c` reste figé).
- `D_us_80180410` (base d'adressage `entityId*4` du dispatch, `EntityUpdates - 4`)
  — référencée par `create_entity.c` lui-même, par `22610.c` et par `e_misc.c`.
  Ajoutée pour de vrai dans `e_init.c` (valeur réelle : le pointeur
  `D_us_80181C86`, la même adresse que la table verticale l'utilise déjà comme
  53ème "slot" fantôme — même pattern d'asymétrie que bo1, 52 entrées réelles
  pour la table verticale, pas 53 comme écrit par erreur au 1er passage).
- `D_us_801804F4` (`g_EInitUnkId13`) — référencée par le propre asm figé
  d'`e_misc.c`. Aliasée comme les 3 précédentes (`g_EInitParticle`,
  `g_EInitInteractable`, `g_EInitCommon`).

**Bonus : bug de transcription détecté et corrigé pendant la relecture** — les
tables `pStObjLayoutHorizontal`/`Vertical` avaient été transcrites à la main la
première fois avec des erreurs réelles (horizontal : une entrée manquante ;
vertical : les 4 dernières entrées dans le mauvais ordre). Regénérées
programmatiquement à partir d'une nouvelle lecture ROM au lieu d'une
transcription manuelle, pour éliminer ce risque.

**3. `BO2_PrizeDrops` non défini** — tout simplement oublié (contrairement à bo1,
aucun `d_prize_drops.c` n'avait été créé). Créé avec les 16 valeurs réelles
extraites de `D_us_80180CA4` (confirmées via `EntityHeartDrop`).

Vérifié en isolation : les 18 fichiers (bo2.h, e_init.c, create_entity.c redevenu
INCLUDE_ASM, d_prize_drops.c + les 14 fichiers migrés) compilent sans erreur.

En attente de confirmation WSL (2ème essai).

**2ème essai WSL : ✅ confirmé.** Build complet (`2583/2583`, `✅ us`). Mêmes
warnings de bruit de fond habituels (bo3/dra, préexistants, sans rapport).

**1er checkpoint de bo2 terminé et confirmé byte-exact.** 14 fichiers migrés
(create_entity exclu, reporté ; e_misc.c exclu, reporté) + bo2.h/e_init.c +
d_prize_drops.c. Reste sur bo2 : `create_entity.c` (migration à reprendre plus
prudemment) et `e_misc.c` (4 fonctions boss-spécifiques à décompiler pour de
vrai) et le code boss-spécifique proprement dit (`22610.c` 24 fonctions,
`337D0.c` 8 fonctions). Puis bo5 et bo7 restent entièrement à faire (aucun
n'a encore ni sa migration ni son code boss-spécifique).

## bo2 — décompilation d'e_misc.c (2026-07-31)

Décision utilisateur : finir bo2 complètement avant de passer à autre chose.

Décompilé les 4 fonctions boss-spécifiques d'`e_misc.c` (entrée 6 de la table de
dispatch + 3 autres, situées entre `UnkCollisionFunc4` et `EntityBigRedFireball`) :
- `func_us_801AF744` : petite entité particule/débris (E_UNK_06), init via
  `g_EInitParticle`, auto-destruction après 0x25 ticks.
- `func_us_801AF844` : entité particule similaire, anime via `D_us_80181680`
  (animFrames transcrit octet pour octet), auto-détruite en fin d'animation.
- `func_us_801AF910` : variante étendue de `CreateEntityFromEntity`
  (create_entity.h) avec des copies de champs supplémentaires propres à ce
  boss (unk5A/zPriority/animSet/flags/palette-ou-hitEffect).
- `func_us_801AF9D8` : fait apparaître 6 entités E_EXPLOSION autour de
  `g_CurrentEntity` avec angle/échelle aléatoires — effet de mort/impact.

Les 18 autres fonctions du fichier sont confirmées identiques à
`src/st/e_misc.h` (aucun autre orphelin), migrées vers l'en-tête partagé.
`func_us_801A9D5C` (un des stubs `create_entity.c` encore non identifiés)
s'est révélé être une vraie fonction appelée depuis `func_us_801AF9D8` avec la
signature `(entityId, source, dest)` — confirme que c'est du vrai code, pas un
artefact, cohérent avec la découverte du 1er essai WSL de ce checkpoint.

Vérifié en isolation : tous les fichiers bo2 compilent toujours sans erreur.

**Ce qui reste, et c'est substantiel** : les 8 fragments `func_us_` de
`create_entity.c` (nécessitent de comprendre leur vrai rôle avant de pouvoir
migrer le fichier), et surtout le code boss-spécifique proprement dit —
`22610.c` (24 fonctions, ~3500 lignes d'asm réparties sur deux très grosses
fonctions de ~1200 lignes chacune + plein de petites) et `337D0.c` (8
fonctions). Ces fonctions partagent un bloc d'état boss local
(`D_us_801B683C`-`D_us_801B68F4`, une trentaine de champs) qu'il faut
cartographier proprement avant d'écrire du code cohérent — comparable en
ampleur au travail déjà fait sur `1CEEC.c` de bo3 (54 fonctions), qui avait pris
plusieurs sessions.

En attente de confirmation WSL (e_misc.c) avant de continuer sur 22610.c/337D0.c.

**Confirmé WSL : ✅.** Build complet (`2583/2583`, `✅ us`). e_misc.c est donc
byte-exact. (Les lignes "deleting src/weapon/gen/..." dans la sortie rsync sont
du nettoyage de fichiers déjà supprimés côté workspace, sans rapport.)

Passage à `22610.c` (24 fonctions).

Décompilé `func_us_801A2610` (garde `g_CurrentEntity` dans les limites
horizontales de la salle, la fait rebondir sur les bords selon le signe de
`velocityX`) — vérifié en isolation.

**Constat important en creusant les fonctions suivantes** : la majorité des 24
fonctions de `22610.c` (+ les 8 de `337D0.c`) partagent un bloc d'état commun
(`D_us_801B683C`-`D_us_801B68F4`) qui s'avère être une table assez complexe —
pas un simple struct plat. Exemple concret : `func_us_801A56D4` calcule une
adresse via `(&D_us_801B684E) + index*4 + 0xA`, qui pour `index=0` tombe
exactement sur `D_us_801B6858` (confirmé : `func_us_801B5C2C` dans 337D0.c
contient les labels bruts `D_us_801B6850/6854/6858/685C` espacés de 4 octets,
donc un vrai tableau de pointeurs 32 bits) — mais le même calcul pour
`index=1` ne tombe PAS sur l'entrée suivante attendue de ce tableau, donc
l'indexation réelle est plus subtile qu'un simple `tableau[index]`. Deviner la
disposition exacte ici serait risqué (le genre d'erreur qu'on veut éviter,
cf. la règle de rigueur du projet) : il faut croiser plus de sites d'appel
avant d'écrire du code définitif.

**Statut honnête** : ce module (24+8 fonctions, ~3500 lignes d'asm, table
d'état partagée pas encore totalement cartographiée) est un travail de
décompilation à part entière, comparable en ampleur à `1CEEC.c` de bo3 (qui
avait pris plusieurs sessions). Je continue dessus méthodiquement plutôt que
de deviner du code potentiellement faux.

## 2026-07-31

Reprise de `22610.c` après compaction de session. J'ai relu intégralement
`func_us_801A269C` (1222 lignes, le gros "chef d'orchestre" IA du boss) pour
en cartographier la structure avant d'écrire quoi que ce soit — c'est un
`switch` sur `self->step`/`step_s` avec plusieurs mini-jump-tables internes,
même famille que `func_us_801A1878` de bo1.

**Découverte utile** : les accès `-0x234($s2)`/`-0x232($s2)` s'expliquent
proprement (`sizeof(Entity) == 0xBC`, et `0x234 == 3 × 0xBC`) — c'est
l'entité *3 emplacements avant* `self` dans `g_Entities` (probablement
l'autre partie du boss). Et `-0x200($s2)` = `(self-3)->flags` (même calcul,
`0x200 = 0x234 - 0x34`). Donc pas un mystère de table globale séparée : juste
un accès direct à une entité voisine dans le tableau. En revanche cette
fonction reste la plus grosse et la plus risquée à transcrire à la main
(beaucoup de valeurs de registres qui survivent d'un label à l'autre via les
delay slots — un test rapide a montré qu'une lecture naïve donnait le mauvais
résultat, `self->palette = 0x815F` et non une valeur "collée" d'un test
précédent). Je la laisse donc de côté pour l'instant et j'avance sur les
fonctions plus tractables du même fichier, pour construire le contexte
(notamment sur le bloc d'état partagé `D_us_801B68xx`) avant d'y revenir.

Décompilé et vérifiés en isolation (gcc -fsyntax-only) cette session :
`func_us_801A5460`, `func_us_801A5678`, `func_us_801A5910`,
`func_us_801A3AC8`, `func_us_801A3BB4`, `func_us_801A3CF0`,
`func_us_801A4BA4`, `func_us_801A4D4C` (8 fonctions, toutes des entités
"particule/projectile" boss-specific assez simples). Nouveau détail sur le
bloc `D_us_801B68xx` : `func_us_801A5460` prouve que `D_us_801B6850` et
`D_us_801B6852` sont deux champs `u16` indépendants (et non un seul pointeur
32 bits) — corrige une hypothèse pas encore vérifiée de la session
précédente.

Pas encore de build WSL pour ces 8 fonctions (à faire au prochain point de
contrôle). Reste dans `22610.c` : `func_us_801A3818`, `func_us_801A3E04`,
`func_us_801A460C`, `func_us_801A4EE0`, `func_us_801A54B4`,
`func_us_801A56D4`, `func_us_801A578C`, `func_us_801A593C`,
`func_us_801A5BE8`, `func_us_801A5CC8`, `func_us_801A5DCC`,
`func_us_801A6EF8`, `func_us_801A7340`, `func_us_801A75B4`, et
`func_us_801A269C` (différé). Plus les 8 fonctions de `337D0.c`.

**Suite (même jour)** : 7 fonctions de plus décompilées et vérifiées en
isolation : `func_us_801A5BE8` (cheat/debug, libère des primitives sur un
bouton précis), `func_us_801A75B4` (appelle deux fois `func_us_801A9CE8` sur
un bouton), `func_us_801A56D4`, `func_us_801A5CC8`, `func_us_801A54B4`,
`func_us_801A578C` (rendu de texte bitmap-font via une chaîne de
primitives), plus la fonction déjà comptée `func_us_801A5460`.

**Correction importante** : `D_us_801B6858` (et son tableau de 5, parfois 6
avec `D_us_801B686C`) ne pointe PAS vers des `Entity`, mais vers des
`Primitive` (`g_PrimBuf`) — confirmé sans ambiguïté par `func_us_801A54B4`
qui les peuple via `g_api.AllocPrimitives(PRIM_SPRT, ...)` et
`&g_PrimBuf[primIndex]`, chaînés via `->next`. Les largeurs de champs
(`sb`/`sh` à 0x18/0x19/0x1A/0x26/0x32) correspondaient par coïncidence aux
largeurs des champs `Entity` (blendMode/drawFlags/scaleX/entityId/
entityRoomIndex), ce qui avait initialement induit en erreur — les vrais
champs sont `u1/v1/tpage/priority/drawMode` de `Primitive`. Deux fonctions
déjà écrites (`func_us_801A56D4`, `func_us_801A5CC8`) ont été corrigées en
conséquence avant de continuer. Leçon : ne pas se fier à la largeur d'un
champ seule pour identifier sa structure — chercher un site qui construit
le pointeur lui-même.

Total `22610.c` : 15/24 fonctions faites (gcc-vérifiées, pas encore WSL).
Reste : `func_us_801A3818`, `func_us_801A3E04`, `func_us_801A460C`,
`func_us_801A4EE0`, `func_us_801A593C`, `func_us_801A6EF8`,
`func_us_801A7340`, `func_us_801A5DCC` (1155 lignes), et
`func_us_801A269C` (différée, voir plus haut). Plus `337D0.c` (8 fonctions,
pas commencé).

**Suite (même jour, 2e lot)** : `func_us_801A3818` décompilée et vérifiée.
Révèle un nouveau pattern self-relatif : `self - 1` (l'entité juste avant
dans `g_Entities`) est le "conducteur" du boss (vérifié via
`prev->entityId == 0x17 == E_UNK_17 == func_us_801A269C`) — cette
sous-entité (hitbox de membre) copie sa position/orientation depuis lui et
lui renvoie un signal via un octet à l'offset 0x90 (`ext+0x14`), le même champ
"tour de rôle" déjà repéré dans `func_us_801A269C`. Nouveau helper ajouté à
`bo2.h` : macro `ENTITY_UNK90(e)` pour accéder proprement à ce champ sur
`self` ou une entité voisine sans dupliquer les casts partout — servira
aussi pour la grosse fonction différée.

Total `22610.c` : 16/24 fonctions faites (gcc-vérifiées). Reste :
`func_us_801A3E04`, `func_us_801A460C`, `func_us_801A4EE0`,
`func_us_801A593C`, `func_us_801A6EF8`, `func_us_801A7340`,
`func_us_801A5DCC` (1155 lignes), et `func_us_801A269C` (différée).

**Suite (même jour, 3e lot)** : `func_us_801A7340` et `func_us_801A593C`
décompilées et vérifiées. La 2e est un petit interpréteur bytecode horodaté
(pilote un script d'entités dans `g_Entities_64` via `D_us_801B6880`) — a
révélé une NOUVELLE correction : `D_us_801B6880` est un vrai pointeur
(`u8*`, curseur de script), pas un `s32` comme supposé initialement à partir
de `func_us_801A5910` seul (qui ne faisait que stocker une valeur biaisée
sans révéler le vrai type pointé). `func_us_801A5910` corrigée en
conséquence.

Total `22610.c` : 18/24 fonctions faites (gcc-vérifiées, pas encore WSL).
Reste : `func_us_801A3E04` (543 lignes), `func_us_801A460C` (382),
`func_us_801A4EE0` (380), `func_us_801A6EF8` (298), `func_us_801A5DCC`
(1155), et `func_us_801A269C` (différée, 1222). Plus `337D0.c` (8
fonctions, pas commencé).

**Suite (même jour, 4e lot)** : `func_us_801A6EF8` décompilée et vérifiée —
c'est le contrôleur de la cutscene de défaite du boss (verrouille l'input
joueur via `g_Player.padSim`/`demo_timer`, fait marcher le joueur vers une
position scriptée). Contient un vrai `jr` sur une table de sauts à 8
entrées (`jtbl_us_801A2018`) — au lieu de deviner les limites de chaque
`case` à partir du flux de contrôle seul, j'ai lu les 8 adresses réelles
directement dans `asm/us/boss/bo2/data/0.data.s` pour éviter toute erreur
de découpage (une leçon du travail sur la grosse fonction différée).
Découverte notable : `g_Status.wornEquipment[1] == ITEM_AXE_LORD_ARMOR`
(0x19) saute une copie d'animation redondante — c'est le check de la voie
"vraie fin" (armure de Richter), cohérent avec le lore réel de SOTN pour ce
combat.

Total `22610.c` : 19/24 fonctions faites (gcc-vérifiées, pas encore WSL).
Reste : `func_us_801A3E04` (543 lignes), `func_us_801A460C` (382),
`func_us_801A4EE0` (380), `func_us_801A5DCC` (1155), et `func_us_801A269C`
(différée, 1222). Plus `337D0.c` (8 fonctions, pas commencé).

**Suite (même jour, 5e lot)** : `func_us_801A460C` décompilée et vérifiée —
contrôleur d'un "flash" plein écran joué à la mort du boss. Alloue 2
primitives PRIM_GT4 utilisées uniquement comme primitives PRIM_ENV
(changement d'environnement de dessin) : la première couvre l'essentiel de
l'écran (rect y=0x14, w=0x100, h=0xCF) via SetDrawEnv, la seconde restaure
l'environnement suivant dans la liste d'affichage. Fait un fondu (step_s
0/1/2, ~30 frames) puis rétrécit le rect de clip vers self->posX avant de
faire apparaître une entité de fumée/débris (entityId 0x29) avec un décalage
aléatoire, tant que `D_us_80180AB0` reste à 0.

Découverte clé : `g_api.func_800EDB08(prim)` (déjà implémentée dans
`src/dra/4DA70.c`) enregistre elle-même le pointeur DR_ENV* retourné DANS
la primitive, à l'adresse de son champ `r1` (`*(DR_ENV**)&prim->r1 = dr;`)
— ce qui explique un `lw` apparemment absurde à l'offset 0x10 d'une
primitive plus loin dans la fonction : c'est en fait une relecture de ce
DR_ENV* mis en cache, pas un champ de couleur. Ajout de 3 macros
`ENTITY_UNK7C/80/88` dans bo2.h (même logique que ENTITY_UNK90 : les
structs ET_Werewolf/ET_Minotaur existants ne nomment pas ces champs de
façon cohérente entre les deux variantes) et de l'extern `D_us_80180AB0`.

Total `22610.c` : 20/24 fonctions faites (gcc-vérifiées, pas encore WSL).
Reste : `func_us_801A3E04` (543 lignes), `func_us_801A4EE0` (380),
`func_us_801A5DCC` (1155), et `func_us_801A269C` (différée, 1222). Plus
`337D0.c` (8 fonctions, pas commencé).

**Suite (même jour, 6e lot)** : `func_us_801A4EE0` décompilée et vérifiée —
c'est le "chef d'orchestre" qui positionne les deux entités visibles du
combat (self+1 et self+2, via l'idiome désormais familier des offsets bruts
0xBC/0x178 = 1x/2x sizeof(Entity)) : elles sont retaguées entityId=0x23 et
décalées en miroir selon self->params (0 vs non-nul = un côté d'écran ou
l'autre). Les steps 2/4 font glisser self->posX/posY vers le centre sur un
minuteur (ENTITY_UNK80, réutilisé du lot précédent), step 0x10 fait
clignoter self->palette tant qu'un flag de cutscene est actif, step 0xFF
est le même stepper de frame de debug (g_pads_1_pressed 0x80/0x20) que
`func_us_801A4D4C` avait déjà introduit.

Nouvelle macro `TILEMAP_RAW16(off)` ajoutée à bo2.h : les steps 1/3
écrivent des halfwords à g_Tilemap+0x1F6..+0x2F6, bien au-delà de la
struct Tilemap réelle (0x54 octets) — confirmé via symbols.us.txt qu'aucun
symbole n'existe dans cette plage (ça tombe dans g_BgLayers[], sans qu'on
sache encore quel champ précis). Même logique que ENTITY_UNK90 : accès brut
plutôt que d'inventer un découpage BgLayer non vérifié.

Total `22610.c` : 21/24 fonctions faites (gcc-vérifiées, pas encore WSL).
Reste : `func_us_801A3E04` (543 lignes), `func_us_801A5DCC` (1155), et
`func_us_801A269C` (différée, 1222). Plus `337D0.c` (8 fonctions, pas
commencé).

**Suite (même jour, 7e lot)** : `func_us_801A3E04` (543 lignes, la plus
grosse fonction traitée ce lot) décompilée et vérifiée. Les chaînes de
debug elles-mêmes le confirment : "set_step %x\n" sur self->step et
"boss_go %x\n" sur D_us_80180AAC — c'est le chef d'orchestre du
déclenchement de fin/défaite du boss. Alloue un quad plein écran noir
(PRIM_G4, fade to black) à l'étape 0, attend que le joueur soit assez
proche (ou que D_us_801B68EC&0x10 soit levé) pour lancer la musique de fin
(0x31D) à l'étape 1, positionne deux blocs de données de la taille d'une
Entity (D_80078618[0]/[1], via func_us_801A9CE8) comme pseudo-entités du
générique à l'étape 2, positionne les deux parties visibles du boss
(self+1/self+4, entityId 0x17/0x1B) à l'étape 3, vérifie les flags
"CastleFlags vrai fin" à l'étape 4 (pose D_us_80180AAC=1 si oui), fait
cycler les deux parties du boss à travers une table de configuration
D_us_80180AB4[self->0x8C] aux étapes 5/6/7 (étape 7 avance l'index selon le
signe du produit des distances joueur↔chaque partie -- littéralement un
test "de quel côté est le joueur"), puis à l'étape 8 coupe la musique,
fait apparaître une entité de transition (entityId 0x24) et attend
g_api.func_80131F68() avant de relancer le thème du boss.

Nouvelles structures dans bo2.h : `BossPartInitEntry` (table
D_us_80180AB4[], 0x10 octets/entrée -- deux champs sont relus tantôt en
u16 individuel tantôt en mot de 32 bits combiné selon la fonction
appelante, les deux formes sont gardées), `D_80078618[]` (Entity), macros
`ENTITY_UNK84/8C/8D/8E` (même logique que ENTITY_UNK90).

Total `22610.c` : 22/24 fonctions faites (gcc-vérifiées, pas encore WSL).
Reste : `func_us_801A5DCC` (1155 lignes) et `func_us_801A269C` (différée,
1222). Plus `337D0.c` (8 fonctions, pas commencé).

**Vérification WSL (même jour)** : `bash tools/sync-to-wsl.sh && make build -j`
lancé par l'utilisateur → `✅ us`. Confirme que les 22 fonctions de
`22610.c` + tous les ajouts de `bo2.h` de cette session compilent et
linkent sans erreur dans la vraie toolchain (aucune référence non
définie, aucun symbole dupliqué). Seuls warnings présents : des
redéclarations conflictuelles préexistantes dans `1CEEC.c` (bo3, déjà
100% fini, sans rapport avec ce lot) et deux warnings d'assembleur mineurs
dans `src/dra/692E8.c` — aucun des deux ne bloque le build ni ne concerne
bo2.

## 2026-07-31 — `bo2` : `func_us_801A5DCC` (22610.c, la plus grosse fonction du projet à ce jour)

Décompilée et vérifiée en lint syntaxique (WSL en attente). 1155 lignes
d'asm, ~460 lignes de C -- le chef d'orchestre de la cutscene d'intro du
boss, avec sa propre petite machine virtuelle à bytecode (25 opcodes).

Le script bytecode est l'un des deux tableaux statiques `D_us_80181DC5`/
`D_us_80181CCC` (choisis à l'étape 0 selon `self->params` -- fin normale vs
fin vraie/armure de Richter), lu via une nouvelle macro `BOSS_SCRIPT_CURSOR`
qui réutilise `D_us_801B6840` comme curseur `u8*` : `func_us_801A54B4`
(déjà décompilée) fait juste `D_us_801B6840 = (s32) self` de façon
générique avec quel que soit le pointeur qu'on lui passe -- lui passer un
des deux tableaux de script (cast en `Entity*`, uniquement pour coller au
type du paramètre) transforme donc `D_us_801B6840` en curseur dans ce
tableau. C'est une **deuxième instance indépendante** du même "un s32
contient en fait un pointeur" déjà vu avec `D_us_801B6880`/
`func_us_801A5910` (l'autre VM à bytecode du fichier, sur `g_Entities_64` --
sans rapport avec celle-ci).

Les 25 opcodes couvrent : animation du texte partagé (op 1, via
`func_us_801A56D4`), configuration d'un sprite deux tons (op 5, tables
`D_us_80180B7C/80/84`), gestion de bits d'un masque partagé `D_us_801B68EC`
(ops 16/17/22/24, avec rembobinage du curseur si le bit attendu n'est pas
encore levé -- un pattern "attendre puis réessayer la prochaine frame"),
un saut de script encodé sur 4 octets empaquetés en nibbles (op 15, et op
14 qui calcule d'abord une cible via une table avant de faire pareil),
`LoadTPage`/`PlaySfx`/`func_us_801A5910` (ops 19/20/12), et un pseudo-opcode
"dessiner une tuile" pour toute valeur brute >= 0x19 (position encodée
directement dans les nibbles de l'octet d'opcode, sauf la valeur spéciale
0x20 qui avance juste le curseur sans rien dessiner).

Les étapes 2-7 qui suivent le script gèrent : l'attente d'un rétrécissement
de primitive (`func_us_801A5CC8`, étape 2), le glissement en/hors champ
d'une paire de primitives `D_us_801B686C`/next (étapes 3/4, qui reviennent
à l'étape 1 ou avancent selon `D_us_801B6850`), la construction/démontage
d'une jauge HUD de 72 tuiles (étape 5, sous-états 0/1/2 via `self->step_s`),
un compte à rebours "effacement" (étape 6), et enfin l'étape 7 qui pose les
**vrais drapeaux de fin** (`g_CastleFlags[0xB4]/0x85/0x62` + bits
`g_Settings+0x10C` pour la vraie fin, ou `D_us_80180B78` + relance du
thème du boss pour la fin normale) avant de détruire `self`.

Nouveaux ajouts `bo2.h` : macro `BOSS_SCRIPT_CURSOR`, `D_us_80181DC5[]`/
`D_us_80181CCC[]` (les deux scripts), `D_us_80180B78`, `D_us_801B68F0`,
`D_us_801A1F84` (chaîne de format FntPrint), `D_us_80180B7C/80/84/88/8C/90`
(petites tables de correspondance).

Total `22610.c` : 23/24 fonctions faites. Ne reste que le chef d'orchestre
différé `func_us_801A269C` (1222 lignes). Puis `337D0.c` (8 fonctions, pas
commencé).

**Vérification WSL (2026-07-31)** : `make build -j` → `✅ us` après correction
d'un doublon (le stub `INCLUDE_ASM("boss/bo2/nonmatchings/22610",
func_us_801A5DCC)` était resté juste après la vraie définition de la
fonction, causant une erreur d'assembleur "symbol already defined" au
premier essai -- corrigé en supprimant le stub résiduel). Confirme que
`func_us_801A5DCC` compile et link sans erreur dans la vraie toolchain.
Warnings restants (`1CEEC.c`, `692E8.c`) préexistants, sans rapport avec
bo2.

## 2026-07-31 — `bo2` : `func_us_801A269C`, dernière fonction de `22610.c` (24/24)

Décompilée et vérifiée (WSL `✅ us`). 1223 lignes d'asm -- le "chef
d'orchestre" principal du combat (entité E_UNK_17), une grosse machine à
états sur `self->step` (valeurs 0,1,2,7,8,0xB,0xC,0xE,0x10,0x12,0x14,0x16,
0x18,0x21,0x22,0x40,0xFF -- chacune une phase/attaque du combat), avec
plusieurs sous-machines à états sur `self->step_s` à l'intérieur de
certains steps (notamment 0x16, 0x40, 0xC qui ont leurs propres jump
tables `jtbl_us_801A1E98/1EB0/1EC8/1EE0`).

Découverte clé : `self - 3` est utilisé ABONDAMMENT dans cette fonction,
mais PAS comme "entité voisine dans le tableau" façon `func_us_801A3E04`
(self+1/self+4) -- ici c'est plus probablement une deuxième copie du
conducteur lui-même (Minotaure vs Loup-garou, les deux variantes du boss
partageant cette même fonction à un offset de slot fixe), lu pour son
`posX.i.hi`, ses `flags`, et son propre `ENTITY_UNK8F`. Piège de lecture
évité de justesse : le "aiguillage externe" du gros switch initial charge
souvent une valeur constante dans `$v0` juste avant de sauter vers un
`case`, et cette valeur PERSISTE (via les délais de branchement,
toujours exécutés) une fois arrivé dans le corps du `case` -- un
`case 0x21` semblait comparer `step_s` contre `0x21` lui-même (non-sens),
jusqu'à réaliser que le slot de délai du `beq` externe recharge en fait
`$v0=1` juste avant, donc la vraie comparaison est `step_s==1`. Leçon
généralisable à revérifier sur toute fonction bo restante avec un gros
if/else en cascade suivi de jump table.

Nouveaux ajouts `bo2.h` : `ENTITY_UNK82/8F/9C` (macros ext+6/+0x13/+0x20),
`D_us_80180530` (EInit), `D_us_80180536` (palette), ~24 tables
AnimateEntity/UnkCollisionFunc (`D_us_801806xx`/`807xx`/`808xx`), la table
d'épilogue partagée `D_us_80180908`/`D_us_80180880` (indexée par
`self->step`, même forme 4-octets que `func_us_801A3818`), 3 chaînes
FntPrint de debug, `g_Entities_160`/`g_Entities_224`, `PLAYER_velocityX`/
`PLAYER_facingLeft`.

**`22610.c` est maintenant 24/24 -- terminé et vérifié WSL.** Reste
`337D0.c` (8 fonctions) avant que bo2 soit complet.

## 2026-07-31 — `337D0.c` : 5 fonctions sur 8

Bascule de `#include "common.h"` vers `#include "bo2.h"`. Cinq fonctions
décompilées et vérifiées (gcc isolé, 0 erreur) :

- `func_us_801B37D0` : variante de `func_us_801A2610` (rebond aux bords de
  la salle), même idiome mais marge 8px et clamps 0x1A8/0x58 au lieu de
  0x10px/0x1A0/0x60.
- `func_us_801B4F5C` : traqueur de hitbox lié à l'entité précédente
  (`self - 1`), copie conforme du pattern déjà établi par
  `func_us_801A3818`, mais surveille `entityId == 0x1B` (E_UNK_1B =
  `func_us_801B385C`, l'autre "conducteur" de cette moitié d'overlay) au
  lieu de 0x17.
- `func_us_801B503C` : lit un `Entity*` lié stocké à ext+0x7C -- **un
  troisième usage distinct de ce même offset** (le premier était le
  `Primitive*` de `func_us_801A460C`, le second est le `Primitive*` de
  `func_us_801B52FC` ci-dessous) ; confirmé par les offsets déréférencés
  (+8 velocityX, +0x34 flags, tous deux trop grands pour un Primitive de
  0x24 octets) plutôt que supposé. Gardé en cast local plutôt que
  réutilisé via `ENTITY_UNK7C` pour ne pas mélanger deux types de
  pointeur sous une même macro.
- `func_us_801B5218` : entité flash/pulse, fondu d'opacité -8/frame,
  auto-destruction à 0.
- `func_us_801B52FC` : alloue 3 prims PRIM_GT4 chaînés, stocke la tête de
  chaîne à ext+0x7C (ici un vrai `Primitive*`, via `ENTITY_UNK7C`), spawn
  optionnel de 2 entités E_UNK_1F via `func_us_801A9D5C`.

**Leçon reconfirmée** : deux fonctions différentes peuvent réutiliser le
même offset ext ambigu (0x7C) pour des types totalement différents
(`Entity*` vs `Primitive*`) -- toujours vérifier via l'arithmétique des
offsets déréférencés plutôt que de supposer une réutilisation identique
du pattern déjà vu ailleurs dans le fichier.

`func_us_801B5C2C` s'est révélée être un stub vide (nop; jr ra) -- traduite
directement.

**Bloqueur identifié sur `func_us_801B56C0`** : fonction marquée
`/* Handwritten function */` dans le `.s` (seule occurrence de ce tag dans
tout le repo). Utilise `$s5` à la fois comme pointeur (`sw $t4, 0($s5)`,
résultat d'un AVSZ4/OTZ) ET comparé numériquement (`slti $v0, $s5, 0x81`)
SANS jamais l'initialiser dans la fonction -- ni dans le prologue, ni
ailleurs dans le corps. Ce n'est pas un piège classique de delay-slot ;
soit (a) c'est un registre pinné à travers plusieurs fonctions de cet
overlay par convention (un curseur OT/Z partagé, jamais réinitialisé),
soit (b) c'est un bug réel du jeu d'origine (lecture d'un registre
jamais mis à jour). Aucune des deux hypothèses n'est vérifiable sans
retrouver where $s5 est réellement posé ailleurs dans l'overlay -- pas
tenté cette session, laissée en `INCLUDE_ASM` en attendant.

**Bloqueur `func_us_801B56C0` résolu** : trouvé son jumeau exact,
`EntityWerewolfDeathFlames` dans `src/st/rare/e_werewolf.c` (même effet --
chaîne de prims "death flame" lancés radialement, animés via une table de
sprites indexée par octet -- pour le Loup-Garou normal plutôt que le
boss). Ce jumeau confirme que `gte_stszotz(otz)` (passer la variable
`long otz` PAR VALEUR, pas `&otz`) est un idiome déjà établi et accepté
sous VERSION_US pour cette macro précise -- pas un bug propre à notre
fonction. `$s5` est simplement `otz` elle-même sous cette convention.
Décompilée en miroir exact du jumeau, avec les constantes propres à la
variante boss (durée de vie 0x140 vs 0x100, fondu r0 -2/frame vs -1,
clamp couleur 0x81/0x80 vs 0x60/0x40, table `D_us_801A16F0` en octets
vs `sprites_rare_4` en u16).

**`337D0.c` est maintenant 7/8** -- gcc isolé, 0 erreur. Ne reste que
`func_us_801B385C` (1593 lignes, probable second "conducteur", analogue
à `func_us_801A269C`, tracké par `func_us_801B4F5C` via `entityId==0x1B`).

## 2026-07-31 — `func_us_801B385C` décompilée : `337D0.c` complet (8/8)

Dernière fonction du fichier, et la plus grosse (1593 lignes d'asm, lue
intégralement en 6 passes). Confirmée comme le second "conducteur" boss
(`E_UNK_1B`), miroir exact de `func_us_801A269C` (`22610.c`) : même
architecture (gros switch sur `self->step`, plusieurs steps avec leur
propre sous-machine `step_s`/table de sauts, menu debug caché au step
`0xFF`), et son épilogue partagé appelle `func_us_801B37D0()` (le
rebondisseur de bords de salle en tête de ce fichier) exactement comme
`func_us_801A269C` appelle `func_us_801A2610()`.

**Idiome `self±3` confirmé symétrique** : `func_us_801A269C` utilise déjà
`self-3` pour lire une seconde copie de lui-même (l'autre variante du
boss). Ici, `func_us_801B385C` utilise `self+3` (= `self + 0x234` octets,
`3 * sizeof(Entity)` avec `sizeof(Entity) == 0xBC`) pour référencer l'AUTRE
conducteur -- les deux conducteurs se référencent donc mutuellement, un
dans chaque sens.

Steps décompilés : `0/1` (init + intro avec sous-machine à 4 états),
`2` (idle/agro), `4` (déplacement ciblé sur le joueur), `6` (saut),
`8` (sous-machine à 3+ états avec spawn d'entité `E_UNK_1D`), `0xA`
(recul), `0xC` (transition), `0xE` (sous-machine à 4 états, flash de
`blendMode`), `0x12` (rotation + spawn de débris `E_UNK_1E`), `0x14`
(sous-machine à 4 états, vol + rotation), `0x16` (sous-machine à 8 états
-- attaque de projectile complète : vol, largage, calcul d'angle vers
`g_Entities` via `GetAngleBetweenEntities`/`rcos`/`rsin`, collision,
retour), `0x18` (sous-machine à 4 états, rotation + spawn `E_UNK_1A`),
`0x20` (mort/désactivation), `0xFF` (menu debug, miroir de celui de
`func_us_801A269C`).

Quelques zones à confiance réduite, signalées par des commentaires dans
le code plutôt que "résolues" par supposition :
- `D_us_80180AB0 = (s32) D_us_80181858` (case 1/step_s 2) : store brut
  d'une adresse de table dans un champ flag, même idiome que
  `BOSS_SCRIPT_CURSOR` déjà vu ailleurs dans ce fichier.
- Le `diff` de case `0x16`/step_s 1 n'est recalculé que si
  `self->facingLeft != 0` -- comportement transcrit littéralement (pas
  "corrigé") pour matcher l'asm brut.
- La constante de rotation `-0x180` en case `0x16`/step_s 4 : gardée
  telle quelle après double vérification contre l'asm.

20 nouveaux symboles ajoutés à `bo2.h` (3 chaînes `FntPrint`, 15 tables
`AnimateEntity`/`UnkCollisionFunc`, la paire `D_us_801819F8`/`D_us_80181958`
pour la hitbox par pose -- même forme que `D_us_80180908`/`D_us_80180880`
de `func_us_801A269C`).

**`337D0.c` est maintenant 8/8, `bo2` (Minotaur/Loup-Garou) est donc
100% décompilé côté code boss-spécifique.** Vérifié via gcc isolé sur le
fichier complet (`gcc -fsyntax-only -std=gnu99 -I include -I include/psxsdk
-I . -D VERSION_US src/boss/bo2/337D0.c`) : **0 erreur**, seulement les
avertissements tolérés habituels (implicit-declaration pour
`AnimateEntity`/`MoveEntity`/`PlaySfxPositional`/`UnkCollisionFunc2`/
`UnkCollisionFunc3`/`UnkPolyFunc0`/`UnkPolyFunc2`/`SetStep`/`SetSubStep`/
`GetSideToPlayer`/`GetDistanceToPlayerX`/`GetAngleBetweenEntities`, plus
les builtins/version.h habituels).

Reste : build WSL réel (`bash tools/sync-to-wsl.sh && cd ~/sotn-decomp
&& rm -rf build && make build -j`) à faire confirmer par l'utilisateur
pour la vérification byte-exact finale de `bo2` en entier -- seule preuve
réelle, le gcc isolé ne fait que valider la syntaxe. `create_entity.c`
(tâche #45, 8 stubs) reste séparée/optionnelle, non bloquante pour
déclarer `bo2` fini.

**Build WSL confirmé (2026-07-31)** : `make build -j` → `✅ us`. Ni
`22610.c` ni `337D0.c` n'apparaissent dans les quelques avertissements du
log (tous pré-existants et sans rapport : conflits de types dans
`1CEEC.c`/bo3, déjà fini, et un warning assembleur `$at` dans
`src/dra/692E8.c`) -- confirmation que le lot bo2 complet compile et
link proprement, sans référence non définie ni symbole dupliqué.
**`bo2` (Minotaur/Loup-Garou) est donc officiellement terminé côté code
boss-spécifique.** Seul `create_entity.c` (8 stubs, tâche séparée,
optionnelle) reste en suspens, sans bloquer la suite.

## 2026-07-31 — `create_entity.c` (bo2) : les 8 "stubs mystères" résolus, fichier terminé

Tâche #45 reprise : les 8 fonctions `func_us_` sans nom dans `create_entity.c` (repérées lors de la
migration checkpoint 1, voir plus haut) ne sont PAS des fonctions distinctes. En relisant l'asm brut
de chacune (6 lignes chacune) et en croisant avec les fonctions "propres" voisines
(`FindFirstEntityAbove`, `CreateEntitiesAbove`, `InitRoomEntities`, `UpdateRoomPosition`,
`CreateEntityFromCurrentEntity`, `CreateEntityFromEntity`), il s'est avéré que ce sont exactement la
PREMIÈRE INSTRUCTION de la fonction "propre" juste après elles dans le fichier — `splat` a mal
placé la frontière de fonction un octet trop tôt à 8 endroits, presque toujours parce que le
compilateur avait hissé un chargement de constante (`ori $a1,$zero,0xfffe`) ou l'ajustement de pile
(`addiu $sp,$sp,-0x20`) juste avant le "vrai" début visible de la fonction suivante.

Confirmé de deux façons indépendantes : (1) arithmétique d'adresse (chaque stub + la taille de son
unique instruction == l'adresse de début de la fonction nommée suivante), (2) recoupement des sites
d'appel — rien EN DEHORS de `create_entity.c` n'appelle jamais `func_us_801A9808`/`9854`/`98AC`/
`99A8`/`9ABC`/`9C34` ; et `UpdateRoomPosition` lui-même appelle `func_us_801A98AC`/`func_us_801A99A8`
là où le header partagé `create_entity.h` appellerait `CreateEntitiesAbove`/`CreateEntitiesBelow`
directement — preuve que ces noms bruts SONT ces fonctions, juste référencées par l'adresse de leur
toute première instruction plutôt que par leur symbole "propre".

**Découverte annexe (bug dans du travail déjà marqué fini)** : `func_us_801B5C2C`, écrite comme une
fonction vide dans `337D0.c` lors d'une session précédente, s'est révélée être en réalité de la DATA
(BSS) — le début du bloc de 4 variables scratch privées de `create_entity.c`
(`g_LayoutObjHorizontal`/`Vertical`/`PosHorizontal`/`PosVertical`, mêmes noms que dans le header
partagé `src/st/create_entity.h`). Un grep complet du repo confirme que rien n'appelle jamais cette
adresse comme du code — seule `create_entity.c` la lit/écrit comme un pointeur. C'est exactement le
piège documenté dans `reference_decomp_verification.md` : la BSS n'étant pas stockée dans le binaire
compilé, un `✅ us` peut passer même si une adresse BSS est mal comprise/mal typée en C, parce que ça
n'affecte pas le SHA1 de sortie. Cette fonction fantôme dans `337D0.c` est du code mort (jamais
appelée) donc inoffensive telle quelle, mais mérite un nettoyage dans une session dédiée plutôt que
d'être touchée en passant ici (pour ne pas déstabiliser `337D0.c` sans re-vérification complète).

**Fichier réécrit en entier** (les 15 vraies fonctions, aucune n'était en fait déjà "propre" malgré
les apparences — les 23 lignes du fichier d'origine étaient TOUTES encore en `INCLUDE_ASM`) :
`CreateEntityFromLayout`, `CreateEntityWhenInVerticalRange`, `CreateEntityWhenInHorizontalRange`,
`FindFirstEntityToTheRight`, `FindFirstEntityToTheLeft`, `CreateEntitiesToTheRight`,
`CreateEntitiesToTheLeft`, `FindFirstEntityAbove`, `FindFirstEntityBelow`, `CreateEntitiesAbove`,
`CreateEntitiesBelow`, `InitRoomEntities`, `UpdateRoomPosition`, `CreateEntityFromCurrentEntity`,
`CreateEntityFromEntity` — toutes vérifiées ligne à ligne contre l'asm réel, logique identique au
header partagé `create_entity.h` (jamais `#include`-é directement puisque ce fichier a ses propres
adresses de stockage locales fixes, `D_us_801B5C2C/30/34/38`, ajoutées à `bo2.h`). `D_us_8018026C`/
`D_us_80180340` (tables de layout horizontal/vertical, déjà définies dans `e_init.c`) également
externées dans `bo2.h`. `func_us_801A9CE8`/`func_us_801A9D5C` (les 2 des 8 adresses réellement
référencées depuis `22610.c`/`337D0.c`) aliasées via des macros dans `bo2.h` vers
`CreateEntityFromCurrentEntity`/`CreateEntityFromEntity` plutôt que de garder deux symboles pour la
même fonction.

Vérifié via gcc isolé (0 erreur) sur `create_entity.c` ET re-vérifié `22610.c`/`337D0.c`/`e_misc.c`/
`e_init.c` (0 erreur chacun, rien cassé par les changements de `bo2.h`).

**`create_entity.c` est maintenant 15/15 — terminé.** Reste : build WSL réel pour confirmer
l'octet-exactitude du fichier + de tout `bo2` avec ce dernier morceau inclus.

---

## 2026-07-31 (suite) — bo5 (Hippogryph) : 6 premières fonctions, la "chaîne de quads" du 1FD30.c

Reprise après la mise en place du fork/remotes GitHub. bo5 était 100% vierge (37 fonctions,
toutes en `INCLUDE_ASM` dans `1FD30.c`). Approche habituelle : commencer par les plus petites
fonctions du fichier, en remontant leurs dépendances jusqu'à ce qu'un groupe cohérent soit
entièrement dérivé.

**6 fonctions écrites et vérifiées via gcc isolé (0 erreur)** :
`func_us_801A19CC`, `func_us_801A4A4C`, `func_us_801A666C`, `func_us_801A66B0`,
`func_us_801A6B74`, `func_us_801A6B94`.

Le cœur du groupe est `func_us_801A66B0` (320 lignes d'asm) : une machine à états à 10 pas
(`self->step` 0-9, table de saut `jtbl_us_8019F940` retrouvée telle quelle dans
`asm/us/boss/bo5/data/0.data.s` pour fixer précisément quel bloc de code correspond à quel pas —
indispensable ici vu qu'il y a 7 blocs de code distincts pour 10 entrées de table, donc plusieurs
pas partagent le même bloc). Elle gère une chaîne de 4 primitives `Primitive` (quads GT4, `sizeof
0x34`, chaînées via leur champ `next`) allouée par `g_api_AllocPrimitives(PRIM_GT4, 4)` — un
effet visuel de traînée (aile ou queue) qui "sort" (pas 1-4, delta positif) puis "rentre" (pas
6-8, delta négatif) via `func_us_801A666C`, qui décale les 4 coins d'un quad. Pas 5 = pause
(jusqu'à 0xC0 appels). Pas 9 = rétraction finale puis `DestroyEntity`. Les deux petites fonctions
`func_us_801A6B74`/`func_us_801A6B94` ne sont que des wrappers (`side=0`/`side=1`).

Point de vigilance résolu : `func_us_801A666C` opère bien sur un enregistrement `Primitive` du
tableau global `g_PrimBuf` (indexé par `self->primIndex`, retrouvé via `include/primitive.h`,
champs x0-x3/y0-y3 correspondant exactement aux offsets vus en asm) — PAS sur `Entity*` comme
envisagé avant d'avoir lu `func_us_801A66B0` en entier. Un compteur `self->ext.utimer.t` (champ
générique `ET_UTimerOnly`, déjà existant dans `entity.h`) sert de minuteur par pas.

Deux zones encore non nommées, documentées en commentaire plutôt que devinées : le flag global
`D_us_801B5538` (son vrai rôle sera clarifié quand `func_us_801A4E40`, le "chef d'orchestre" de
1136 lignes qui l'écrit, sera décompilée), et l'octet `ext+0x7D` touché par `func_us_801A19CC`
(pas de struct `Ext` existante ne correspond, donc accès brut par pointeur avec commentaire plutôt
que d'inventer un nom de champ). Ajout de `#include <stage.h>` en tête de `1FD30.c` (absent
jusqu'ici) pour que `Entity`/`Primitive`/`g_CurrentEntity`/`g_PrimBuf` soient visibles.

**Reste dans 1FD30.c** : 31 fonctions sur 37, dont les grosses `func_us_801A1C14` (2156 lignes,
probable chef d'orchestre principal), `func_us_801A4E40` (1136 lignes) et `func_us_801A0490`
(782 lignes).

Vérifié via gcc isolé seulement pour l'instant — build WSL réel à faire pour confirmer
l'octet-exactitude de ce premier lot de 6 fonctions avant de continuer.

---

## 2026-07-31 (suite 2) — bo2 : le build WSL a révélé 2 bugs de link dans du travail "fini"

Premier vrai build WSL depuis le fork : `bobo2.elf` a échoué au link (bo5 lui-même était gcc-clean,
sans rapport). Deux causes distinctes, toutes deux dans `create_entity.c` (session précédente) :

**1. `func_us_801B5C2C` mal nommé en `D_us_801B5C2C`.** Le bug BSS documenté la session précédente
(la fonction vide dans `337D0.c`) était réel, mais le renommage choisi était faux : le disassembly
brut de `create_entity.c` (généré par splat depuis le vrai ROM) référence déjà cette adresse sous le
nom `func_us_801B5C2C` partout (lw/sw, jamais jal — confirmé data, jamais code). En le redéclarant
sous un nouveau nom `D_us_801B5C2C` dans `bo2.h`/`create_entity.c` sans que personne ne définisse
réellement ce symbole, le linker ne trouvait ni l'un ni l'autre. Vérification supplémentaire cette
fois : le fichier asm complet `func_us_801B5C2C.s` fait plus de 3300 octets, tous `nop` (0x00000000),
de 0x801B5C2C à 0x801B6930+ — un immense bloc BSS, pas une fonction à 2 instructions comme la lecture
partielle de la session précédente le laissait penser. Corrigé : `337D0.c` définit maintenant
`u16* func_us_801B5C2C;` (le vrai nom splat, le bon type), `bo2.h`/`create_entity.c` mis à jour en
conséquence.

**2. Table `D_us_80180000` (encore en asm brut) référence `func_us_801A9ABC`/`func_us_801A9C34`.**
Ces 2 des 6 adresses "mal découpées" (voir la session précédente) n'étaient aliasées que via des
macros `#define` dans `bo2.h` — ce qui marche pour les appels C (22610.c/337D0.c) mais PAS pour
`D_us_80180000`, qui est un fichier `.s` brut (`asm/us/boss/bo2/data/0.data.s`, un
`AbbreviatedOverlay` pas encore porté en `header.c` comme bo4/bo6/mar/rbo0 l'ont déjà fait) : un
`#define` ne touche jamais un fichier asm, il faut un vrai symbole pour le linker. Corrigé sans
toucher à l'asm ni risquer un futur portage de `D_us_80180000` en `header.c` : deux directives
`__asm__(".global ...; .set ...")` en fin de `create_entity.c`, testées isolément avec l'assembleur
hôte (résultat confirmé : mêmes adresses que `InitRoomEntities`/`UpdateRoomPosition`).

Les deux corrections vérifiées via gcc isolé (0 erreur) sur `337D0.c`, `create_entity.c`, `22610.c`,
`e_init.c`, `e_misc.c`. Un futur `AllocEntity`/`g_pads`/`AnimateEntity`/`SquareRoot0` "conflicting
types" dans `1CEEC.c` (bo3) est apparu dans le log WSL mais n'a PAS fait échouer le build (juste des
warnings, ninja a continué) — à surveiller dans une session dédiée à bo3, pas urgent.

**Reste à faire : relancer le build WSL** pour confirmer que ces deux fix résolvent bien le link de
`bobo2.elf`, avant de considérer bo2 vraiment clos.

---

## 2026-07-31 (suite 3) — bo2 : ✅ us confirmé, les 2 fix de link tiennent

Rebuild WSL complet après les 2 corrections (`func_us_801B5C2C` renommé correctement + alias linker
pour `func_us_80180000`) : `✅ us`. Mêmes warnings bo3/1CEEC.c et bo2/692E8.c qu'avant (non
bloquants, déjà notés). **bo2 est maintenant vraiment clos** : boss-specific (22610.c + 337D0.c) +
create_entity.c, tout byte-exact confirmé. bo5 en cours (6/37 dans 1FD30.c, gcc-clean, ce lot n'a pas
encore eu son propre tour de build WSL dédié mais fait partie de ce `✅ us` global).

---

## 2026-07-31 (suite 4) — bo5 : 5 fonctions de plus (11/37)

`func_us_801A459C` (init d'un petit bloc d'état `D_us_801B554x`, aucun paramètre),
`func_us_801A47B4` (construit un `RECT` sur la pile et appelle `ClearImage`, confirmé via
`include/psxsdk/libgpu.h`), `func_us_801A4430` (init hitbox une fois au step 0, mêmes offsets
Entity que déjà établis), `func_us_801A5F28` (fait glisser `g_unkGraphicsStruct.BottomCornerTextTimer`
— confirmé par calcul d'offset exact dans `game.h` — vers une cible, +-1/frame, la comparaison ne
regarde que les 16 bits bas du compteur s32 alors que l'incrément/decrément touche les 32 bits
complets, un idiome déjà vu ailleurs pour ce compilateur non optimisant), `func_us_801A1BA0`
(repousse g_CurrentEntity si le tilemap a trop scrollé — a un paramètre `s16* sensors` transmis tel
quel à `UnkCollisionFunc3`, repéré uniquement parce que rien ne remplissait `$a0` avant le `jal`).

Point méthodo à noter : `func_us_801A1BA0` a d'abord été écrite à tort comme `void(void)` — l'absence
de tout `lui/addiu` pour `$a0` avant un premier `jal` est le signal qu'il faut chercher, ça veut dire
le paramètre de la fonction elle-même est transmis tel quel à l'appelée.

Vérifié via gcc isolé (0 erreur) — la session a redémarré entre-temps (le mirror isolé `/tmp/repo_iso2`
avait disparu, recréé par rsync direct du repo réel, donc ce check porte sur le fichier réel).
**Reste 26/37** dans `1FD30.c`. WSL à lancer pour confirmer ce nouveau lot.

---

## 2026-07-31 (suite 5) — bo5 : ✅ us confirmé pour le lot de 5 (11/37)

Rebuild WSL : `✅ us`, mêmes warnings pré-existants qu'avant (bo3/1CEEC.c, dra/692E8.c), rien de neuf
côté bo5. Lot de 5 confirmé byte-exact. On enchaîne sur la suite.

---

## 2026-07-31 (suite 6) — bo5 : 5 fonctions de plus (16/37)

`func_us_801A4810`/`func_us_801A4D3C` (un système de "compteur à chiffres" à 5 slots de
`Primitive*` — `D_us_801B5554[5]`, indexé par `D_us_801B554A` — confirmé via les offsets exacts de
`Primitive` : tpage/clut/u0/v0/u1/v1/x0/y0/priority/drawMode, tous retrouvés dans
`include/primitive.h`. `func_us_801A4D3C` fait défiler la coordonnée V du chiffre courant tout en
décalant physiquement (`y0`) tous les AUTRES chiffres — un effet d'odomètre), `func_us_801A1010`
(init unique des 2 "parties" du boss à `self+1`/`self+2` — confirmé que `Entity` fait exactement
`0xBC` octets donc `self+1` en arithmétique de pointeur `Entity*` tombe juste), `func_us_801A4494`
(machine à états de transition de fin de combat : coupe la musique, attend l'API de fade, joue les
sons de victoire — table de saut `jtbl_us_8019F838` relue directement dans `0.data.s`, révèle que le
case 0 tombe en fallthrough dans le case 1 plutôt que d'avoir son propre bloc séparé), `func_us_801A037C`
(init hitbox + spawn conditionnel d'une entité satellite via `AllocEntity(g_Entities_224, ...+0x1780)`
— confirmé `g_Entities_224` comme pool de 32 `Entity` via ce même calcul de taille).

Vérifié via gcc isolé (0 erreur, repo isolé reconstruit par rsync direct après un nouveau redémarrage
du sandbox). **Reste 22/37** dans `1FD30.c`. WSL à lancer.

---

## 2026-07-31 (suite 7) — bo5 : ✅ us confirmé (16/37)

Rebuild WSL : `✅ us`, rien de neuf. Lot de 5 confirmé byte-exact.

---

## 2026-07-31 (suite 8) — bo5 : 5 fonctions de plus (20/37) + correction d'un champ mal identifié

`func_us_801A1884` (barre HP/jauge : 5 quads GT4 croissants de 0x3E de large, texture unie),
`func_us_8019FD30` (première fonction du fichier : step!=0 = AnimateEntity + réaction aux dégâts
[son, spawn item, ReplaceBreakableWithItemDrop], step==0 = init hitbox/priorité/animSet via 4 tables
indexées par `(params>>12)&0xffff`), `func_us_801A3E78` (débris/projectile : step 0 lance avec
vélocité selon facingLeft/params, step 1 applique traînée+rotation+rétrécissement+fondu jusqu'à la
fin de l'anim puis DestroyEntity), `func_us_801A10DC` (chaîne de 2 quads GT4 stockée dans
`self->ext.prim` — un TROISIÈME usage confirmé de cet offset comme `Primitive*`, après les 2 déjà vus
en bo2/bo3), `func_us_801A48C8` (construit une rangée de sprites `PRIM_SPRT` à partir d'une chaîne
d'octets encodée par idx — `0xFF` démarre une séquence d'échappement sur 2 octets qui termine la
chaîne si suivie de 0, tout autre octet non-nul devient un sprite, un octet 0 simple est un "trou"
qui avance la colonne sans consommer de primitive).

**Correction en cours de route** : `g_unkGraphicsStruct` commence en réalité à `0x800973F8`
(confirmé via `config/symbols.us.txt`), PAS `0x800973FC` comme le suggérait le commentaire du premier
champ affiché dans `game.h` (`primIndex` est le tout premier champ, non commenté à cet endroit-là).
Ça décale tout de 4 octets — `func_us_801A5F28` (déjà écrite dans un lot précédent) visait donc le
mauvais champ (`BottomCornerTextTimer` au lieu du vrai `unkC` à l'offset 0x14) ; corrigée dans ce
lot. Point de vigilance à garder : toujours vérifier l'adresse de base RÉELLE d'un struct via
`config/symbols.us*.txt`, pas seulement via le commentaire du premier champ listé dans `game.h`.

Vérifié via gcc isolé (0 erreur). **Reste 17/37** dans `1FD30.c`. WSL à lancer.

---

## 2026-07-31 (suite 9) — bo5 : ✅ us confirmé (20/37), fix unkC inclus

Rebuild WSL : `✅ us`, rien de neuf. Lot de 5 + la correction du champ `g_unkGraphicsStruct.unkC`
confirmés byte-exact.

## 2026-07-31 — `bo5` : 5 fonctions de plus (25/37), gcc-clean, WSL pas encore relancé

Suite du batch précédent (20/37 confirmé `✅ us`). 5 nouvelles fonctions de `src/boss/bo5/1FD30.c` :
`func_us_801A1260` (grille de tuiles GT4, `(params&0xF)` lignes x 3 colonnes, sens gauche/droite
choisi par `params&0x100`), `func_us_801A1404` (panneau de fond fixe + rafraîchissement par frame
des couleurs de palette des 2 `g_GpuBuffers` + repositionnement de son bord haut selon la position Y
du joueur), `func_us_801A19FC` (fait apparaître N débris depuis `g_CurrentEntity`, position/vitesse
aléatoires via `Random()`, ajuste en retour la vitesse X de `g_CurrentEntity` si celui-ci est en
`step 0x16`), `func_us_801A45F0` (init unique d'une icône HUD en 7 sprites chaînés, dont 6 sont
stockés dans le petit bloc d'état `D_us_801B55xx` déjà connu), `func_us_801A425C` (partie
hitbox-only qui suit l'entité juste avant elle dans le tableau (`self-1`), choisit sa forme de
hitbox via une table par frame d'animation `D_us_80180B0C`, se détruit si le parent quitte le type
d'entité `0x22`).

Découverte notable : `D_us_801B5554` (le tableau de 5 `Primitive*` du système de chiffres/score) a
un 6e symbole voisin en mémoire, `D_us_801B5568`, jamais touché par les fonctions déjà écrites
(qui bouclent explicitement `for i<5`) mais bien lu/écrit par `func_us_801A45F0` — gardé comme
variable globale séparée plutôt que d'agrandir le tableau à 5 éléments établi ailleurs.

Tout compile proprement (`gcc -fsyntax-only`, aucun warning nouveau). **Pas encore vérifié par
`make build -j` WSL** — à faire avant de considérer ce lot acquis à 100%.

Reste 12/37 fonctions dans `1FD30.c`, dont les 3 grosses (`func_us_801A1C14` 2156 lignes,
`func_us_801A4E40` 1136 lignes, `func_us_801A0490` 782 lignes).

## 2026-07-31 — `bo5` : 5 fonctions de plus (30/37), gcc-clean, WSL pas encore relancé

Suite du batch précédent (25/37 confirmé `✅ us`). 5 nouvelles fonctions de `src/boss/bo5/1FD30.c` :
`func_us_801A0184` (mini-machine à états d'un piège fixe + partie "porte" liée dans la case
suivante du tableau, deux vérifications de hitbox joueur par frame), `func_us_801A3FD4` (débris
lancé qui devient un marqueur `g_Entities_160` si un bit global de "partie de chaîne" est actif,
sinon spin → attente capteur mur → lancement loin du joueur → chute avec anim), `func_us_801A4A78`
(avance le flux de commandes scripté de `func_us_801A4A4C` : timestamps 16-bit, opcodes spawn/
destroy/wait-flag/set-flag), `func_us_8019FE84` (déclencheur de frontière caméra scripté : élargit/
réduit la boîte de scroll de `g_Tilemap` selon la position X du joueur et une des 15 zones
possibles), `func_us_801A159C` (machine à 12 étapes d'une plateforme de scène liée aux coupures de
musique : tamponne/efface 4 tuiles murales dans `g_Tilemap.fg`, glisse `posY` en virgule fixe brute
entre deux seuils).

Tout compile proprement (`gcc -fsyntax-only`, aucun warning nouveau). **Pas encore vérifié par
`make build -j` WSL.**

Reste 7/37 fonctions dans `1FD30.c` : `func_us_801A0490` (782 lignes), `func_us_801A1C14` (2156
lignes, probablement le chef d'orchestre principal), `func_us_801A3B88` (205 lignes),
`func_us_801A4E40` (1136 lignes, écrit `D_us_801B5538`), `func_us_801A5F88` (225 lignes),
`func_us_801A62B4` (246 lignes). Seulement les 3 grosses (0490/1C14/4E40) restent vraiment
importantes en taille — les 3 autres sont dans la fourchette déjà traitée ce batch.

## 2026-07-31 — `bo5` : défi « les 7 d'un coup » — 6/7 faites, WSL ✅ us confirmé (36/37)

Lolo a lancé le défi de faire les 7 fonctions restantes en une passe sans erreur. Faites dans
l'ordre : `func_us_801A3B88`, `func_us_801A5F88`, `func_us_801A62B4` (les 3 petites, gcc-clean
d'un coup), puis `func_us_801A0490` (782 lignes, fan de plumes du Griffon en GTE : allocation de
`count+6` primitives GT4, boucle de rotation à coordonnées fixes via `rsin`/`rcos`, staging
`DRAWENV`/`POLY_GT4` via `g_api_func_800EDB08`, ressort d'amortissement en virgule fixe entre deux
angles cibles), puis `func_us_801A4E40` (1136 lignes, le pilote de la cutscene de mort : machine à
8 étapes + interpréteur de script à 25 opcodes qui réutilise `D_us_801B553C` — déjà typé `Entity*`
ailleurs dans ce fichier — comme curseur d'octets brut via des casts explicites).

Deux bugs trouvés et corrigés en cours de route : `.raw` au lieu de `.val` sur l'union `f32` dans
`func_us_801A159C`, et une confusion `PLAYER` (l'`Entity` du joueur) / `g_Player` (le `PlayerState`
séparé qui porte réellement `padSim`/`status`/`demo_timer`) dans `func_us_801A5F88`.

Point notable sur `func_us_801A4E40` : son étape 0 appelle `func_us_801A45F0` (déjà confirmé WSL
ailleurs avec la signature `void func_us_801A45F0(Entity*)`) mais lit ensuite une valeur de retour
dans `$v0` — indice que cette fonction renvoie en réalité un booléen de succès (0/1) déjà présent
"gratuitement" dans le registre au moment du `jr $ra`, jamais explicité côté C. Plutôt que de
toucher une fonction déjà byte-exacte ailleurs, l'appel se fait ici via un cast de pointeur de
fonction (`(s32(*)(Entity*))func_us_801A45F0`) pour récupérer cette valeur sans risque.

3 zones marquées LOW/MEDIUM confidence dans les commentaires du code (opcodes 5/14/19 du script :
reconstruction d'adresse par nibbles et paramètres `LoadTPage` pas identifiables avec certitude
depuis la seule trace registre) — tout compile clean malgré tout.

**WSL confirmé `✅ us`** sur ce lot de 6. Reste 1/37 : `func_us_801A1C14` (2156 lignes, la plus
grosse fonction du fichier, probablement le chef d'orchestre principal par frame).

## 2026-07-31 — `bo5` : `func_us_801A1C14` (37/37, gcc-clean, WSL pas encore relancé)

Dernière fonction de `1FD30.c`. Avec 2156 lignes d'assembleur c'est de loin la plus grosse du
fichier — mais heureusement la table de saut principale (`jtbl_us_8019F5E4`, 131 entrées pour
`self->step` 0 à 0x82) ne pointe vers un vrai bloc de code que ~20 fois ; toutes les autres valeurs
de step retombent directement sur l'épilogue partagé (calcul de `hitboxWidth`/`hitboxHeight` selon
des plages de `animCurFrame`). Ça a rendu la fonction gérable : ~20 handlers réels, dont plusieurs
avec leur propre sous-machine à états sur `step_s`, et un step (36, le lancement de la "tornade de
plumes") avec sa propre table de saut interne à 10 entrées (`jtbl_us_8019F7F4`) pour un total de
9 sous-étapes.

Vu l'ampleur des sauts croisés dans l'assembleur d'origine, cette fonction a été traduite en
gardant les labels du binaire d'origine comme cibles de `goto` plutôt qu'en la restructurant en
if/else imbriqués — plus sûr pour préserver le flot de contrôle exact sur une fonction de cette
taille, au prix d'une lisibilité moindre.

Une vraie confusion de champs corrigée en cours de route : plusieurs endroits utilisaient
`self->hitboxOffY`/`self->posY.i.hi` (offsets 0x12 vs 0x6) de façon interchangeable par erreur —
offset 0x6 est en fait `posY.i.hi`, pas `hitboxOffY`. Repéré et corrigé après vérification directe
de `include/game.h`. Autres bugs de compilation trouvés et corrigés : un champ `Ext` inventé
(`.raw32`), plusieurs accès à des champs `Entity`-only (`rotPivotX`, `hitboxWidth`) sur un
`Primitive*` (corrigés en `x1`/`r1`/`r2`, les vrais champs à ces offsets), `func_us_801A1BA0`
(déjà confirmé ailleurs, `void`) rappelé via un cast de pointeur de fonction pour récupérer sa
valeur de succès implicite — même technique que pour `func_us_801A45F0` dans le lot précédent.

Zones marquées MEDIUM/LOW confidence (le reste est solide) : le détail exact de la boucle de
particules "plumes" (step 36, sous-étapes 6/7 — dérive/ressort par plume, simplifié depuis une
trace registre dense), une division par constante via multiplication réciproque (step 18,
sous-étape 3, traduite en `% 48` par analogie avec le pattern habituel plutôt que vérifiée bit à
bit), et un pointeur d'entité (`g_Entities[0]`) réutilisé très loin de sa dernière affectation
claire dans l'assembleur (step 18, bloc `L2C58`).

**bo5 est maintenant à 37/37, tout gcc-clean.** Reste à relancer `make build -j` pour la
confirmation WSL finale de ce fichier.

**WSL confirmé `✅ us`** — `func_us_801A1C14` matche byte-exact malgré les 3 zones flaggées
MEDIUM/LOW confidence (particules du step 36, division traduite en `% 48`, réutilisation tardive de
`g_Entities[0]`). **bo5 (Hippogryph) est officiellement clos : 37/37, WSL byte-exact confirmé.**
Prochain boss : bo7 (Cerberus), `src/boss/bo7/13258.c`, ~14 fonctions, pas encore commencé.

## 2026-07-31 (suite) — bo7 (Cerberus) : 14/14 fonctions traduites, gcc-clean, WSL pas encore relancé

Attaqué `src/boss/bo7/13258.c` juste après la clôture de bo5, sur "enchaine". Les 14 fonctions
(toutes de simples stubs `INCLUDE_ASM` au départ) ont été traduites dans l'ordre croissant de
taille asm, du plus petit (`func_us_80194CE0`, 30 lignes) au plus gros (`func_us_80194D3C`, 889
lignes) :

`func_us_80194CE0` (30 lignes) — pousse `g_CurrentEntity` vers le haut s'il enfonce un capteur de
sol, même convention que les fonctions de collision déjà vues côté bo5 (`UnkCollisionFunc2/3`).

`func_us_801939CC` (65) — petite state machine 2 états (init hitbox + révélation de passage secret
via `g_CastleFlags[0x50]`).

`func_us_801938E0` (68) — anime un `Primitive*` (pas un `Entity*`) : confirmé grâce au commentaire
de `include/primitive.h` sur le champ `p3` ("CHI uses this like a step value" — CHI = nom interne
de ce boss). Bon rappel que tout ce qui ressemble à un state-machine 2 étages n'est pas forcément
sur une `Entity`.

`func_us_801959E0`, `func_us_80195AF0` — deux petits projectiles/débris, rien de notable.

`func_us_801963D8` (172) — 4 sous-états avec un champ `ext8C` (zone `ext` réutilisée comme scratch
s32) pour une dérive de vélocité aléatoire, même pattern déjà vu dans `func_us_801A1C14` (bo5).

`func_us_80193258` (180) — init d'une "seconde entité" empruntée sur le même slot mémoire
(`self+0xBC`, la taille exacte d'une `Entity`), pattern de "sous-entité embarquée" déjà vu ailleurs
dans le projet.

`func_us_80193EF0` (190) — bounce de projectile avec `g_api_CheckCollision` et une vraie division
signée traduite en C naturel (`(-velocityY*2)/3`) plutôt qu'à la main, pour laisser le compilateur
régénérer l'idiome à multiplication réciproque — technique déjà validée cette session sur bo5.

`func_us_801934E8` (268) — tir de boule de feu avec chaîne de 2 primitives GT4 liées (`prim->next`
utilisé pour des champs différents du prim de tête), plus la même division signée naturelle.

`func_us_80193AB0` (301) — jump table 17 entrées (dont 1 seule case réelle + fallback "idle
contrôlé au pad", visiblement un reste de debug) ; dessine des tuiles directement dans
`g_Tilemap.fg` (`u16*`) en plus des primitives.

`func_us_8019477C` (372) — encore un jump table (7 cases), avec chaîne de prims GT4 et calcul de
teinte via `rcos`. **Bug réel trouvé et corrigé pendant la traduction** : `g_Tilemap + 0xA` a été
d'abord mal nommé `scrollY.i.hi` alors que c'est `scrollX.i.hi` (scrollY est à +0xC, pas +0x8) —
corrigé à deux endroits dans cette fonction avant de continuer. Bon rappel de vérifier l'offset
réel dans `include/game.h` plutôt que de faire confiance à un nom copié d'une fonction précédente.

`func_us_801941A0` (393) — init d'une chaîne de 15 "pointes" embarquées (`self+0xBC*i`) plus une
paire de primitives GT4, jump table 9 cases.

`func_us_80195C50` (506) — trigger de zone (musique, TimeAttackController, DRAWENV custom via
`g_api_func_800EDB08`/`SetDrawEnv`, pattern identique à `src/st/e_ctulhu.h` retrouvé et réutilisé
tel quel : `dr_env = (DR_ENV*)LOW(prim->r1)`). Un point flaggé MEDIUM CONFIDENCE : un test
périodique basé sur `g_Timer` utilise une constante magique de division réciproque
(`0x58ED2309`) dont le diviseur exact n'a pas été retrouvé analytiquement — transcrit
littéralement (multiply-high 64 bits + décalages) plutôt que deviné, pour préserver le
comportement même si l'instruction générée ne matche pas à l'octet près.

`func_us_80194D3C` (889 lignes, la plus grosse du fichier) — IA principale de la tête de
Cerberus, ~12 états (`step` 0 à 0xB plus 0xFF pour un mode debug avec `FntPrint`/pad). **Piège de
registre repéré et corrigé avant d'écrire le code** : plusieurs branches `beq v1, v0, .Lxxx` dans
la cascade de dispatch semblaient comparer une sous-étape à une valeur absurde (ex. `step_s == 5`)
à cause d'un registre `v0` réutilisé — en retraçant précisément les delay slots MIPS (qui
s'exécutent TOUJOURS, y compris juste avant un saut pris), la vraie valeur comparée était en fait
`1`, pas `5`. Sans cette relecture attentive des delay slots, plusieurs `if` auraient eu la
mauvaise constante. Un flag MEDIUM/LOW confidence supplémentaire : le sous-état `step_s == 5` bien
réel pour l'étape 5 pointe vers un bloc de démarrage de particules, ordre des sous-états non
strictement séquentiel (0,1,2,3 pour la plupart des steps, mais logique interne cohérente une fois
les delay slots correctement retracés).

**14/14 gcc-clean** (`/tmp/repo_iso2`, rsync rafraîchi, aucune erreur, seulement les warnings
attendus + 1 warning `int-to-pointer-cast` volontaire pour le pattern `LOW()` déjà utilisé ailleurs
dans le projet). **WSL pas encore relancé pour ce lot** — à confirmer par Lolo avec son
`make build -j` local.

## 2026-08-01 — bo7 (Cerberus) : ✅ us confirmé par Lolo (WSL), bo7 CLOSED

`make build -j` complet lancé par Lolo sur son WSL local, build entier (2583/2583 étapes),
résultat final `✅ us` — match byte-exact confirmé pour l'ensemble du build, y compris
`src/boss/bo7/13258.c`. **bo7 (Cerberus) est donc CLOSED, 14/14 fonctions confirmées
byte-exact.**

Le log de build contenait aussi des warnings `conflicting types` sur `src/boss/bo3/1CEEC.c`
(`AllocEntity`, `g_pads`, `D_us_80180588`, `AnimateEntity` x3, `SquareRoot0`, `ET_801A5E4C`) et deux
warnings assembleur (`$at` sans `.set noat`) sur `src/dra/692E8.c`. Ces fichiers ne font pas partie
du lot bo7 et n'ont pas été touchés cette session — ce sont des re-déclarations `extern` locales
dans bo3 qui divergent des prototypes canoniques désormais visibles (probablement introduites lors
du travail bo3 antérieur, avant que ces symboles soient déclarés ailleurs). Comme le build se
termine quand même sur `✅ us`, ce sont des warnings non bloquants qui n'affectent pas le résultat
byte-exact — à nettoyer un jour pour la propreté du code, mais pas urgent.

## 2026-08-01 (suite) — bo0 (Olrox) : démarrage, 2/68 fonctions traduites, gcc-clean

Prochain boss après bo7 : bo0 (Olrox, stage NO2 "Outer Wall"). Contrairement à bo1/2/3/5/7, ce
sandbox n'avait PAS l'asm splitté pour bo0/bo4/bo6 (`asm/us/boss/bo0` était absent). bo4 s'est avéré
déjà 100% fini (0 `INCLUDE_ASM` restant partout) — rien à faire là. bo6 a encore 123 stubs
(`cutscene.c`, `richter.c`, `us_39144.c`, `us_3E79C.c`) mais pas touché pour l'instant. Pour bo0,
`splat64` est installé dans ce sandbox et le disque `disks/us/BOSS/BO0/BO0.BIN` est présent, donc
j'ai relancé `python3 -m splat split config/splat.us.bobo0.yaml` moi-même pour régénérer
`asm/us/boss/bo0/nonmatchings` — ça a marché, 68 fonctions réparties sur 4 fichiers :
`3AB18.c`(1), `3B014.c`(2), `2B9EC.c`(5), `2D26C.c`(60, ~15 300 lignes d'asm à elle seule).

Fait ce lot-ci :
- `func_us_801BAB18` (3AB18.c, 368 lignes) : petite machine à états (7 steps) pour un des segments
  du portail/tunnel d'Olrox, écriture de tuiles dans `g_Tilemap.fg[]` via des tables `D_us_*`,
  mode debug `FntPrint("charal %x\n", ...)` piloté par `g_pads_1_pressed`.
- `func_us_801BB014` (3B014.c, 38 lignes) : simple test de distance au joueur (`|dx|<0x19 &&
  |dy|<0x21`), utilisé comme garde par les fonctions suivantes.
- `func_us_801BB08C` (3B014.c, **897 lignes, la plus grosse du fichier**) : IA principale d'Olrox
  (6 steps + code partagé). Gère l'alloc de 3 primitives GT4 chaînées (`prim->next`) pour l'anneau
  de téléportation, un verrou caméra via `g_api_func_8010E168`/`g_api_func_8010DFF0` +
  `g_Player.padSim`/`g_Player.demo_timer`, une boucle d'anneau tournant (`rcos`/`rsin` sur un angle
  stocké dans un champ `ext` brut à 0x84) avec deux variantes miroir (`ALT` selon bit 0x100 de
  `params`), et un rendu de tuiles de sol final. Traduit littéralement avec des `goto` calqués sur
  les labels asm (même technique que `func_us_80194D3C` en bo7) tellement le flux est dense.

Flag confiance MOYENNE : la formule finale de dessin de tuiles
(`row * g_Tilemap.hSize * 16`) reproduit fidèlement la séquence mult/shift de l'asm mais le sens
exact de `hSize` dans ce contexte n'a pas été vérifié analytiquement — à surveiller si le byte-diff
WSL révèle un écart précisément dans `func_us_801BB08C`.

**2/68 gcc-clean** (`/tmp/repo_iso2`, aucune erreur). Reste 4 fonctions dans `2B9EC.c` (dont une
autre fonction dense de ~420 lignes gérant un état de particule/débris via un `Primitive` en
"linked-list" à 2 éléments) et les 60 fonctions de `2D26C.c`. Pas encore de sitrep WSL demandé pour
ce lot — bo0 est nettement plus gros que bo1/2/3/5/7 réunis, donc on avance par petits lots plutôt
que d'attendre la fin du fichier complet.

## 2026-08-01 (suite 2) — bo0 : 2B9EC.c terminé (7/68 fonctions), gcc-clean

Les 4 fonctions restantes de `2B9EC.c` sont maintenant traduites (7/68 au total pour bo0).
`2B9EC.c` gère les torches/piliers destructibles d'Olrox (3 variantes) et leurs débris :

- `func_us_801AB9EC` : mise à jour par-éclat, appelée une fois par frame par éclat actif depuis les
  3 fonctions ci-dessous pendant qu'elles attendent que les débris se stabilisent. Confirme le motif
  "chaîne de 2 Primitives" (le `next` de la prim de dessin pointe vers une seconde prim utilisée
  comme scratch physique), avec `r3` comme état d'éclat sur 3 plages distinctes (0-7 gros pilier,
  8-11 petit pot, 12-15 pilier tournant) — plages confirmées croisées entre les 3 fonctions
  appelantes.
- `func_us_801ABFE0` : le gros pilier mural avec trou dans le sol en dessous (10 tuiles à
  restaurer). Casse en 8 éclats + fait apparaître 1 entité "récompense" + 8 entités de débris.
- `func_us_801AC54C` : variante pot/urne plus petite (4 éclats, pas de trou au sol).
- `func_us_801AC73C` : la fonction physique par-éclat elle-même (position/vélocité en virgule
  fixe empaquetées via `LOW()`, gravité, minuteur de vie qui masque l'éclat à expiration).
- `func_us_801AC894` : variante "pilier tournant" avec une phase électrique supplémentaire (anneau
  d'environ 30 primitives, `FindFirstUnkPrim`/`g_api_func_800EDB58`) avant de rejoindre le même
  schéma casse/débris que les deux autres.

Toutes des offsets `ext` bruts (0x7C, 0x80, 0x84) transcrits avec des macros locales
`ENTITY_EXT7C/80/84` (même convention que `ENTITY_UNK7C` déjà utilisée en bo2), aucune structure
`Ext` dédiée n'existe encore pour ces entités bo0.

**7/68 gcc-clean.** Reste seulement `2D26C.c` (60 fonctions, ~15 300 lignes) pour clôturer bo0 —
gros morceau restant, mis en pause ici comme convenu avec Lolo pour ne pas enchaîner sans point
d'étape.

## 2026-08-01 (suite 3) — bo0 : build WSL, 2 erreurs de link corrigées, puis bug réel trouvé et corrigé

Premier `make build -j` sur bo0 : deux échecs de link successifs, tous deux liés au mécanisme
`INCLUDE_ASM`/`INCLUDE_RODATA`. Une fois toutes les fonctions d'un `.s` original remplacées par
du C, plus rien ne tire son `.rodata` embarqué dans le link — les chaînes de debug FntPrint
(`D_us_801A9678` dans `3AB18.c`, `D_us_801A92CC` dans `2B9EC.c`) étaient déclarées `extern`
seulement, donc `undefined reference`. Fix : les définir réellement en local. Puis deuxième
échec : `.data` est discardée par le linker script de l'overlay (pas de segment data
inscriptible pour ces overlays ROM) — un `char[]` non-const finit en `.data`. Fix : `const char`.

Après ces deux fix, le build a fini par linker intégralement (2583/2583), mais le check de
checksum a échoué spécifiquement sur `❌ BO0` (tous les autres segments, dont `F_BO0`, en ✅).
Bisection par adresses dans `build/us/bobo0.map` : le drift (~1300 octets) apparaît dès
`func_us_801AB9EC`, le tout premier symbole de `2B9EC.c`.

**Cause réelle, pas juste un problème de taille de code généré** : `func_us_801AB9EC` est en
fait un dispatch à 5 branches (`jtbl_us_801A92B8`, sur `child->u2` = état du fragment de
débris : 0=init, 1=chute/impact, 2=spawn torche-tournante, 3=spawn petit-pot, 4=posé/respawn).
Je n'avais traduit QUE le cas 0 lors du premier passage — les 4 autres cas (spawn de fragments
d'impact via `FindFirstUnkPrim2`/`UnkPolyFunc2`, vélocités aléatoires via `Random()`, respawn
d'entité via `AllocEntity`/`CreateEntityFromCurrentEntity`) manquaient entièrement. D'où l'écart
de taille massif (~296 octets compilés au lieu des ~1524 attendus). Fonction réécrite en entier
(technique goto-label-miroir des adresses originales, cohérente avec les autres gros morceaux),
gcc-clean. Prochaine étape : rebuild WSL + re-check checksum BO0 côté Lolo.

**Leçon retenue** : toujours vérifier qu'un `.s` traduit ne contient pas de table de saut
(`jtbl_*`) avant de considérer une fonction "traduite" à partir d'un seul bloc de code — un faux
sentiment de simplicité sur une fonction "courte" peut cacher un dispatch multi-état.

## 2026-08-01 (suite 4) — bo0 : itération fine sur `func_us_801AB9EC`, plusieurs vrais bugs corrigés

Après la réécriture complète des 5 cas, plusieurs tours d'aller-retour build/asm-differ avec
Lolo ont révélé une succession de vrais problèmes, chacun corrigé :

1. **Switch → jump table réel** : un `switch` C classique à 5 cas ne passe pas le seuil de
   génération de jump table de ce GCC (il compile en chaîne de comparaisons). Forcé via
   l'extension GNU "labels as values" (`static void* const jtbl[] = {&&L1,...}; goto *jtbl[i];`),
   qui reproduit fidèlement `jtbl_us_801A92B8`.
2. **`self->tpage=0xF; self->clut=0x33;` manquants** en tête du cas 0 — omis lors de la
   première transcription, retrouvés par comparaison directe avec le `.s`.
3. **Registres/frame trop gros** (7 registres sauvés + 0x80 vs 6 + 0x68 dans l'original) :
   causé par une variable `child` locale gardée vivante sur toute la fonction (y compris à
   travers des appels comme `FindFirstUnkPrim2`/`g_api_CheckCollision`) alors que l'original
   recharge `self->next` à chaque usage isolé, sans jamais la garder en registre à travers un
   appel. Fix : suppression de la variable partagée, rechargement frais `self->next->...` à
   chaque site d'usage isolé (mais réutilisation locale quand deux écritures consécutives sans
   appel entre les deux, ex. la paire `b2`/`r2`).
4. **`Collider` dupliqué en pile** : cas 1 et cas 4 déclaraient chacun leur propre
   `Collider collider;` alors que l'original partage UN SEUL emplacement pile entre les deux
   (mutuellement exclusifs) — remonté en variable de portée fonction, économise ~36 octets de
   pile.
5. **Ordre des affectations chaînées inversé** — bug transversal important : en C,
   `a = b = c;` stocke **b avant a** (l'affectation interne s'exécute en premier), donc
   `self->u2 = self->u0 = 0xB8;` stocke `u0` puis `u2`, alors que l'asm original stocke `u2`
   PUIS `u0`. Toutes les paires `u2/u0`, `u3/u1`, `v1/v0`, `v3/v2` (cas 0, 2, 3) étaient dans le
   mauvais sens — corrigées (`self->u0 = self->u2 = X;` etc). **Ce même bug a été retrouvé et
   corrigé dans `func_us_801ABFE0`** (bloc GT4 prim, paires `u2/u0`, `u3/u1`, `v1/v0`, `v3/v2`,
   `x2/x0`, `y1/y0`, `y3/y2`) — probablement un biais systématique de ma façon d'écrire ces
   paires, à surveiller sur les fonctions restantes.
6. **Résidu final identifié, non corrigé** : après tous ces fix, il reste un écart d'environ
   20 octets localisé au raccord cas0→tail partagé. Diagnostic précis : ce n'est PAS un bug de
   logique (le contenu généré est fonctionnellement correct) mais une différence de scheduling
   GCC — le compilateur remplit le délai de branchement (`branch delay slot`) du saut du cas 0
   avec l'écriture `b2=8` (valeur déjà connue en registre), puis saute dans un bloc partagé situé
   en fin de fonction qui fait le reste (`r2=8`, `u2=4`, puis l'appel `UnkPrimHelper`) — alors que
   l'original ne fait pas ce découpage. Testé DEUX structures C différentes (goto vers bloc
   partagé avec variables de portée fonction, ET duplication inline manuelle) : les deux
   produisent EXACTEMENT le même résultat compilé, confirmant que c'est une décision de GCC liée
   à la forme du graphe de flux, pas à la façon dont le C est écrit. Mis de côté pour l'instant
   (rendements décroissants) — les 3 autres fonctions de `2B9EC.c` (`801AC54C`, `801AC73C`,
   `801AC894`) restent à vérifier par relecture manuelle plutôt que diff (le diff reste faussé
   tant que ce résidu subsiste, à cause du drift d'adresse résiduel qu'il cause en aval).

**Statut bo0 actuel** : 7/68 fonctions traduites et gcc-clean. `func_us_801AB9EC` et
`func_us_801ABFE0` fonctionnellement correctes et quasi byte-exact (résidu de ~20 octets connu
et documenté sur AB9EC). `func_us_801AC54C`, `func_us_801AC73C`, `func_us_801AC894`,
`3AB18.c`, `3B014.c` pas encore re-vérifiées avec cette rigueur depuis le premier passage —
prochaine étape naturelle si on reprend bo0. `2D26C.c` (60 fonctions) toujours pas commencé.
## 2026-08-01 (suite 5) — RLIB (ST/RLIB) : import de l'overlay, 35/148 fonctions, PR en préparation
Rattrapage : tout le travail RLIB d'aujourd'hui n'avait pas encore été journalisé ici.

**Import de l'overlay** (suivant le runbook wiki du projet) : `ST/RLIB/RLIB.BIN` + `F_RLIB.BIN`
extraits via `tools/sotn_utils/extract_overlay`, confirmés byte-exact contre les disques sources
dès l'extraction. 148 fonctions au total, dont l'extracteur avait déjà nommé la plupart via
correspondance de corpus. 6 paires de symboles ambiguës (fonctions miroir gauche/droite,
au-dessus/en-dessous, angles) désambiguïsées par preuve structurelle (variable globale
spécifique à un axe, forme du graphe de branchement, nom de fonction appelée dans le désassemblage
brut). Une tentative de raccourci (réutiliser des en-têtes partagés d'autres overlays sans
vérification individuelle) a été essayée puis abandonnée : même quand les valeurs/la logique
semblent identiques à un autre overlay, seule une comparaison SHA1 réelle après build WSL fait
foi — la disposition mémoire du compilateur peut différer invisiblement.

**Progression** : 35 fonctions décompilées sur 13 fichiers (`e_lock_camera.c`, `e_candle_table.c`,
`popup.c`, `e_room_bg.c`, `e_room_fg.c`, `e_red_door.c` (partiel), `e_particles.c` (partiel),
`e_explosion_puff_opaque.c` (partiel), `e_stage_name.c` (partiel), `prim_helpers.c`,
`create_entity.c`, `unk_24A44.c`, et aujourd'hui `unk_2DBE8.c` avec `func_us_801AEFE0`).
`func_us_801AEFE0` : petite fonction "suit la position de l'entité Fishhead parente, anime,
détruit si l'animation est terminée" — `self->ext.fishhead.fishheadEntity` (pointeur retour vers
le Fishhead, à l'offset 0x9C) copié dans `posX`/`posY` chaque frame, confirmé byte-exact.

**Régression trouvée et corrigée pendant cette session** : un `rm -rf build` complet (pas le
rebuild incrémental habituel) a révélé que deux fonctions d'`unk_2697C.c`
(`GetAnglePointToEntityShifted`/`GetAnglePointToEntity`, adresses 0x801A697C/0x801A6B10) n'avaient
plus aucun fichier `.s` généré, alors que le journal de travail (`overlay_import_log.txt`) disait
les avoir renommées avec succès plus tôt dans la journée. Cause réelle : leurs entrées dans
`config/symbols.us.strlib.txt` avaient été perdues au fil des multiples cycles de retour en arrière
de la journée (les deux adresses tombaient bien dans de vrais espaces de 144 et 128 octets entre
fonctions déjà séparées — donc pas un vrai problème de détection de limites, juste une régression
de perte de symboles). Corrigé en réinsérant les deux lignes et en relançant
`python3 -m splat split config/splat.us.strlib.yaml`. **Leçon** : un rebuild `rm -rf build`
complet, périodique, est la seule façon fiable d'attraper ce genre de régression — les rebuilds
incrémentaux répétés toute la journée s'appuyaient sans le savoir sur un `.o` en cache qui masquait
le trou.

**Découverte annexe** : `RLIB`/`F_RLIB` ne sont pas encore dans `config/check.us.sha` — le tableau
✅/❌ affiché par `make build` n'a tout simplement pas de ligne pour cet overlay. La seule preuve
réelle est un `sha1sum build/us/RLIB.BIN disks/us/ST/RLIB/RLIB.BIN` (et pareil pour `F_RLIB.BIN`)
manuel — les deux correspondent exactement après le fix ci-dessus.

**Prochaine étape** : préparation d'un PR vers la branche `rlib-decomp-progress` (déjà poussée sur
le fork et review une première fois par l'admin du projet sozud sur Discord, à un stade moins
avancé que maintenant — feu vert donné pour un PR "checkpoint" dès que le build matche). Le travail
actuel étant assis sur la branche `boss-decomp-bo1-bo2-bo3` (mélangé avec le travail bo0/bo2/bo5/bo7
non lié), transfert fait via un `git worktree` isolé plutôt qu'un stash/checkout risqué, pour ne
jamais toucher au travail boss en cours, non commité.
**Statut RLIB actuel** : 35/148 fonctions, 13 fichiers, clean rebuild ✅, prêt pour PR.
## 2026-08-01 (suite 6) — RLIB : faux positif sha1sum trouvé, vrai bug corrigé, func_us_801AEFE0 réellement confirmé
Découverte importante en revérifiant `func_us_801AEFE0` avant de préparer le PR : le fichier
`unk_2DBE8.c` était retourné à son stub `INCLUDE_ASM` nu (perdu pendant un `git stash`/`git stash
pop` fait plus tôt pour isoler le bug `❌ BO0`). Le premier "✅ byte-exact" de cette fonction ne
prouvait donc rien — `INCLUDE_ASM` intègre l'assembleur original brut, qui est byte-parfait par
construction peu importe ce que contient le `.c` autour. **Leçon de méthode : toujours vérifier
(`grep`/`md5sum`) que le fichier source contient bien le C écrit à la main juste avant ET juste
après un build de vérification — un checksum qui passe seul ne suffit pas.**
Après réapplication de la fonction, la revérification (cette fois avec preuve du contenu réel du
fichier avant/après build) a trouvé un vrai bug : `self->posX = self->ext.fishhead.fishheadEntity
->posX; self->posY = ...->posY;` (deux instructions séparées) fait recharger deux fois le pointeur
`fishheadEntity` par ce vieux compilateur PSYQ/GCC-2.x (pas de CSE inter-instructions sur les
chaînes de pointeurs `a->b->c`), confirmé via `tools/asm-differ/diff.py --overlay strlib --
func_us_801AEFE0` montrant un `lw` en trop là où la cible avait juste un `nop`. Fix : sortir le
pointeur dans une variable locale explicite avant de l'utiliser deux fois — match byte-exact
immédiat. **`func_us_801AEFE0` est maintenant réellement confirmé** (SHA1 RLIB.BIN et F_RLIB.BIN
identiques à l'original, vérifiés avec le contenu du fichier confirmé avant/après le build).
