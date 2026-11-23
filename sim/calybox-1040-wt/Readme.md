
| from | len | FF | msgNo | msgType | lenHead | deviceId | net | header | magic | crcHead | payload | crc |
|:-----|:---:|:--:|:-----:|:-------:|:-------:|:--------:|:---:|:------:|:-----:|:-------:|:-------:|:---:|
| tydom | 17 | ff | 0a | 00 | 0c | 12 34 56 | c0 | 85 98 00 | 25 7b | fb 6b | 32 01 ff ff ff | 9c 8a |
| caly  | 17 | ff | 02 | 00 | 0c | ce 4d e4 | 04 | 85 98 00 | a4 02 | fc 3a | 3a 05 12 34 56 | 00 00 |
| tydom | 17 | ff | 0b | 05 | 0c | 12 34 56 | c0 | 05 98 00 | da d0 | fa e1 | 32 00 ce 4d e4 11 00 ff ff | f0 d8 |



The whole exchange, in order:

| from | len | FF | msgNo | msgType | lenHead | deviceId | net | header | magic | crcHead | payload | crc |
|:-----|:---:|:--:|:-----:|:-------:|:-------:|:--------:|:---:|:------:|:-----:|:-------:|:-------:|:---:|
| me | 17 | ff | 02 | 00 | 0c | 28 5c 15 | c0 | 85 98 00 | 1e 45 | fd 27 | 32 01 ff ff ff | 00 3f |
| me | 17 | ff | 02 | 00 | 0c | 28 5c 15 | c0 | 85 98 00 | 1e 45 | fd 27 | 31 01 ff ff ff | ee ed |
| me | 17 | ff | 02 | 00 | 0c | 28 5c 15 | c0 | 85 98 00 | 1e 45 | fd 27 | 30 01 ff ff ff | 44 bc |
| Calybox | 17 | ff | bd | 00 | 0c | ce 4d e4 | **04** | 85 98 00 | 7e 7f | fb e3 | **3a 05 28 5c 15** | 36 55 |
| Calybox | 17 | ff | bd | 00 | 0c | ce 4d e4 | 04 | 85 98 00 | 7e 7f | fb e3 | 39 05 28 5c 15 | d8 87 |
| Calybox | 17 | ff | bd | 00 | 0c | ce 4d e4 | 04 | 85 98 00 | 7e 7f | fb e3 | 38 05 28 5c 15 | 72 d6 |



CALYBOX 1020 WT
  "discovery": "rt2012_noOutTemp_X3D_x3d_pped"
    "profile": "rt2012_noOutTemp",
    "protocol": "X3D",
    "type": "x3d_pped",

RF 6600 FP
  "discovery": thermic_X3D_x3d_rm
    "profile": "thermic",
    "protocol": "X3D",
    "type": "x3d_rm"



17 ff 07 00 0c d7 d9 68 c0 85 98 00 25 7b fb 6b 32 01 ff ff ff 9a b4
17 ff 07 00 0c d7 d9 68 c0 85 98 00 25 7b fb 6b 31 01 ff ff ff 74 66
17 ff 07 00 0c d7 d9 68 c0 85 98 00 25 7b fb 6b 30 01 ff ff ff de 37
17 ff 02 00 0c cb 4d e4 04 85 98 00 e5 b0 fb 4e 3a 05 d7 d9 68 67 97
17 ff 02 00 0c cb 4d e4 04 85 98 00 e5 b0 fb 4e 39 05 d7 d9 68 89 45
17 ff 02 00 0c cb 4d e4 04 85 98 00 e5 b0 fb 4e 38 05 d7 d9 68 23 14
Start: device_data_task
1b ff 08 05 0c d7 d9 68 c0 05 98 00 da d0 fa e1 32 00 cb 4d e4 11 00 ff ff 26 62
1f ff 03 05 0c cb 4d e4 04 05 98 00 de 00 fc 85 3a 00 d7 d9 68 11 80 03 00 01 e5 01 05 ae 71
19 ff 09 05 0c d7 d9 68 c0 05 98 00 39 d1 fb 81 32 00 cb 4d e4 10 00 83 07
19 ff 0a 05 0c d7 d9 68 c0 05 98 00 e8 d1 fa d2 32 00 cb 4d e4 10 00 45 05
1b ff 0b 05 0c d7 d9 68 c0 05 98 00 d9 d0 fa e2 32 00 cb 4d e4 11 04 01 00 05 eb
1b ff 0c 05 0c d7 d9 68 c0 05 98 00 5e c7 fb 66 32 00 cb 4d e4 11 04 02 00 aa 53


reg: 528 sub: 1024
32 00 ce 4d e4 10 02 00 04 f0 
reg: 784 sub: 1024
32 00 ce 4d e4 10 03 00 04 00 

| tydom       | calybox | info |
|:------------|:--------|:-----|
| 11 00 ff ff | 11 80 03 00 01 e5 01 05 | 17: 3 Phase? e5(229) Voltage? |
| 10 00       | 10 80 30 30 00 3f 00 00 00 00 00 00 00 00 00 00 00 02 00 00 00 | 16: ???  2 = number heaters? |
| 11 04 01 00 |	11 84 01 00 39 07 | 1041-1: 1849 or 18.49 |
| 11 04 02 00 |	11 84 02 00 3f 07 39 07 | 1041-2: 1855 or 18.55, 1849 or 18.49 |
| 11 01 01 00 |	11 81 01 00 f6 22 | 273-1: 8950 |
| 11 01 02 00 |	11 81 02 00 ff 27 f6 22 | 273-2: 10239, 8950 ? on time of heaters?|
| 12 01 01 00 |	12 81 01 00 1c 08 | 273-1: 2076, 20.76 room temp |
| 12 01 02 00 |	12 81 02 00 00 80 1c 08 | 273-2: 0x8000 = no temp, 2076, 20.76 room temp |
| 10 02 00 04 f0 | ??? | |
| 10 03 00 04 00 | ??? | |

| tydom       | calybox | info |
|:------------|:--------|:-----|
| 11 44 01 00 38 07 | ??? | 1041: turn off whole heating
| 11 43 01 00 f1 07 | ??? | 785-1: heater 1 power safe
| 11 43 01 00 f6 07 | ??? | 785-1: heater 1 combined mode
| 11 43 01 00 f4 07 | ??? | 785-1: heater 1 defrost
| 11 43 02 00 00 00 f1 07 | ??? | 785-2: heater 2 power safe
| 11 43 02 00 00 00 f6 07 | ??? | 785-2: heater 2 combined mode
| 11 43 02 00 00 00 f4 07 | ??? | 785-2: heater 2 defrost


10 82 
00 04 10 
6b 16 1a 
6
 b = 11
16 = 22
1a = 26
ff ff ff ff 
b5 2d 05 00 = 339.381kWh month 10
09 31 03 00 = 209.161kWh month 11
ff ff ff ff 
ff ff ff ff 
ff ff ff ff 
ff ff ff ff 
ff ff ff ff

10 83 
00 04 00 
6
 b = 11
16 = 22
1a = 26
9a 4d 01 00  85402
ff ff ff ff 
84 9e 00 00  40580
42 c2 00 00  49730
a9 82 00 00  33449
