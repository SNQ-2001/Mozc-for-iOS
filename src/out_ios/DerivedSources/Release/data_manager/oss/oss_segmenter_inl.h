
namespace  {
const int kLSize = 2575;
const int kRSize = 2575;

bool IsBoundaryInternal(uint16 rid, uint16 lid) {
  // BOS * or * EOS true
  if (rid == 0 || lid == 0) { return true; }
  // 名詞,数,アラビア数字 名詞,数,(アラビア数字|漢数字) false
  if (((rid == 1952)) && ((lid == 1952) || (lid >= 1954 && lid <= 1963))) { return false; }
  // 名詞,数,漢数字 名詞,数,アラビア数字 true
  if (((rid >= 1954 && rid <= 1963)) && ((lid == 1952))) { return true; }
  // 名詞,数,漢数字 名詞,数,漢数字 false
  if (((rid >= 1954 && rid <= 1963)) && ((lid >= 1954 && lid <= 1963))) { return false; }
  // 名詞,数,(アラビア数字|漢数字) 名詞,数,区切り文字 false
  if (((rid == 1952) || (rid >= 1954 && rid <= 1963)) && ((lid == 1953))) { return false; }
  // 名詞,数,区切り文字 名詞,数,(アラビア数字|漢数字) false
  if (((rid == 1953)) && ((lid == 1952) || (lid >= 1954 && lid <= 1963))) { return false; }
  // 接頭詞,数接続 名詞,数,漢数字 false
  if (((rid == 2552)) && ((lid >= 1954 && lid <= 1963))) { return false; }
  // 記号,(句点|読点|括弧開|括弧閉) * true
  if (((rid >= 2555 && rid <= 2558)) && (true)) { return true; }
  // * 記号,(句点|読点|括弧開|括弧閉|空白) true
  if ((true) && ((lid >= 2555 && lid <= 2558))) { return true; }
  // ^フィラー * true
  if (((rid >= 2 && rid <= 11)) && (true)) { return true; }
  // * ^フィラー true
  if ((true) && ((lid >= 2 && lid <= 11))) { return true; }
  // * ^動詞,非自立,*,*,*,*,(あげる|上げる|つづける|続ける|そこねる|そびれる|おえる|終える|はじめる|始める|ねがえる|願える|もらえる) true
  if ((true) && ((lid == 865) || (lid >= 873 && lid <= 874) || (lid == 877) || (lid == 883) || (lid == 886) || (lid == 888) || (lid == 890) || (lid >= 896 && lid <= 897) || (lid >= 899 && lid <= 900) || (lid >= 908 && lid <= 909) || (lid == 912) || (lid == 918) || (lid == 921) || (lid == 923) || (lid == 925) || (lid >= 931 && lid <= 932) || (lid >= 934 && lid <= 935) || (lid >= 943 && lid <= 944) || (lid == 947) || (lid == 953) || (lid == 956) || (lid == 958) || (lid == 960) || (lid >= 966 && lid <= 967) || (lid >= 969 && lid <= 970) || (lid >= 978 && lid <= 979) || (lid == 982) || (lid == 988) || (lid == 991) || (lid == 993) || (lid == 995) || (lid >= 1001 && lid <= 1002) || (lid >= 1004 && lid <= 1005) || (lid >= 1013 && lid <= 1014) || (lid == 1017) || (lid == 1023) || (lid == 1026) || (lid == 1028) || (lid == 1030) || (lid >= 1036 && lid <= 1037) || (lid >= 1039 && lid <= 1040) || (lid >= 1048 && lid <= 1049) || (lid == 1052) || (lid == 1058) || (lid == 1061) || (lid == 1063) || (lid == 1065) || (lid >= 1071 && lid <= 1072) || (lid >= 1074 && lid <= 1075) || (lid >= 1083 && lid <= 1084) || (lid == 1087) || (lid == 1093) || (lid == 1096) || (lid == 1098) || (lid == 1100) || (lid >= 1106 && lid <= 1107) || (lid >= 1109 && lid <= 1110) || (lid >= 1118 && lid <= 1119) || (lid == 1122) || (lid == 1128) || (lid >= 1131 && lid <= 1132) || (lid == 1134) || (lid >= 1140 && lid <= 1141) || (lid >= 1143 && lid <= 1144) || (lid >= 1152 && lid <= 1153) || (lid == 1156) || (lid == 1162) || (lid >= 1165 && lid <= 1166) || (lid == 1168) || (lid >= 1174 && lid <= 1175) || (lid == 1177))) { return true; }
  // * ^動詞,非自立,*,*,*,*,(いただく|頂く|ぬく|抜く) true
  if ((true) && ((lid == 1192) || (lid == 1196) || (lid == 1199) || (lid >= 1203 && lid <= 1204) || (lid == 1208) || (lid == 1211) || (lid >= 1215 && lid <= 1216) || (lid == 1220) || (lid == 1223) || (lid >= 1227 && lid <= 1228) || (lid == 1232) || (lid == 1235) || (lid >= 1239 && lid <= 1240) || (lid == 1244) || (lid == 1247) || (lid >= 1251 && lid <= 1252) || (lid == 1256) || (lid == 1259) || (lid >= 1263 && lid <= 1264) || (lid == 1268) || (lid == 1271) || (lid >= 1275 && lid <= 1276) || (lid == 1280) || (lid == 1283) || (lid == 1287))) { return true; }
  // * ^動詞,非自立,*,*,*,*,(いたす|致す|つくす|尽くす|なおす|直す) true
  if ((true) && ((lid == 1343) || (lid >= 1345 && lid <= 1346) || (lid >= 1348 && lid <= 1351) || (lid >= 1353 && lid <= 1354) || (lid >= 1356 && lid <= 1359) || (lid >= 1361 && lid <= 1362) || (lid >= 1364 && lid <= 1367) || (lid >= 1369 && lid <= 1370) || (lid >= 1372 && lid <= 1375) || (lid >= 1377 && lid <= 1378) || (lid >= 1380 && lid <= 1383) || (lid >= 1385 && lid <= 1386) || (lid >= 1388 && lid <= 1391) || (lid >= 1393 && lid <= 1394) || (lid >= 1396 && lid <= 1398))) { return true; }
  // * ^動詞,非自立,*,*,*,*,(こむ|込む) true
  if ((true) && ((lid >= 1399 && lid <= 1414))) { return true; }
  // * ^動詞,非自立,*,*,*,*,(おわる|終わる|いらっしゃる|らっしゃる|下さる|くださる|クダサル) true
  if ((true) && ((lid == 1433) || (lid == 1453) || (lid == 1472) || (lid == 1489) || (lid == 1508) || (lid == 1527) || (lid == 1546) || (lid == 1565) || (lid == 1584) || (lid == 1603) || (lid >= 1623 && lid <= 1625) || (lid >= 1627 && lid <= 1630) || (lid >= 1632 && lid <= 1635) || (lid >= 1637 && lid <= 1640) || (lid >= 1645 && lid <= 1646) || (lid >= 1648 && lid <= 1650) || (lid >= 1652 && lid <= 1655) || (lid >= 1657 && lid <= 1660) || (lid >= 1662 && lid <= 1665) || (lid >= 1667 && lid <= 1670) || (lid >= 1672 && lid <= 1675) || (lid == 1677) || (lid == 1679))) { return true; }
  // * ^動詞,非自立,*,*,*,*,(あう|合う|願う|もらう) true
  if ((true) && ((lid == 1682) || (lid >= 1689 && lid <= 1690) || (lid >= 1692 && lid <= 1693) || (lid >= 1699 && lid <= 1700) || (lid >= 1702 && lid <= 1703) || (lid == 1710) || (lid == 1712) || (lid >= 1714 && lid <= 1715) || (lid >= 1722 && lid <= 1723) || (lid >= 1725 && lid <= 1726) || (lid >= 1733 && lid <= 1734) || (lid >= 1736 && lid <= 1737) || (lid == 1744) || (lid == 1746) || (lid >= 1748 && lid <= 1749) || (lid >= 1756 && lid <= 1757) || (lid == 1759))) { return true; }
  // 接頭詞,丁寧連用形接続 動詞,*,*,*,*,丁寧連用形 false
  if (((rid == 2516)) && ((lid == 561) || (lid == 571) || (lid == 601) || (lid >= 620 && lid <= 625) || (lid == 680) || (lid == 693) || (lid == 702) || (lid == 711) || (lid >= 719 && lid <= 722) || (lid == 767) || (lid == 778) || (lid >= 786 && lid <= 789) || (lid == 819) || (lid == 835))) { return false; }
  // * ^名詞,接尾可能 true
  if ((true) && ((lid >= 1948 && lid <= 1949))) { return true; }
  // ^助詞,接続助詞 ^動詞,非自立 false
  if (((rid >= 320 && rid <= 355)) && ((lid >= 845 && lid <= 1764))) { return false; }
  // ^動詞,自立 ^動詞,非自立 false
  if (((rid >= 559 && rid <= 844)) && ((lid >= 845 && lid <= 1764))) { return false; }
  // 副詞,一般,*,*,*,*,よく 動詞,自立,*,*,五段・ラ行,*,なる false
  if (((rid == 14)) && ((lid == 721) || (lid == 725) || (lid == 729) || (lid == 733) || (lid == 737) || (lid == 741) || (lid == 745) || (lid == 749) || (lid == 753) || (lid == 757) || (lid == 761) || (lid == 765))) { return false; }
  // 名詞,接尾,サ変接続 動詞,自立,*,*,サ変・スル false
  if (((rid >= 1859 && rid <= 1865)) && ((lid >= 601 && lid <= 613))) { return false; }
  // * 動詞,非自立,*,*,五段・カ行促音便,連用タ接続,く false
  if ((true) && ((lid == 1320))) { return false; }
  // ^接頭詞 ^接頭詞 true
  if (((rid >= 2516 && rid <= 2552)) && ((lid >= 2516 && lid <= 2552))) { return true; }
  // * 名詞,非自立,形容動詞語幹,*,*,*,みたい false
  if ((true) && ((lid == 2105))) { return false; }
  // * ^助詞,*,*,*,*,*,(ヲ|ニ|ナ|ネ|ヨ|ン|ヨー|ワ|デ|ノ|ヘ|ヲ|之|ナア|ネェ|ヨー|なァ) true
  if ((true) && ((lid >= 308 && lid <= 310) || (lid >= 369 && lid <= 373) || (lid == 390))) { return true; }
  // * 名詞,非自立,副詞可能,*,*,*,(きり|ため|っきり|はず|ほど|まま) false
  if ((true) && ((lid == 2043) || (lid >= 2048 && lid <= 2049) || (lid >= 2059 && lid <= 2061))) { return false; }
  // * 名詞,非自立,副詞可能 true
  if ((true) && ((lid >= 2038 && lid <= 2099))) { return true; }
  // * 名詞,非自立,一般,*,*,*,(コト|フシ|ホ|モノ|ン) true
  if ((true) && ((lid == 2004) || (lid >= 2007 && lid <= 2010))) { return true; }
  // * 助動詞,*,*,*,特殊・デス,基本形,ッス true
  if ((true) && ((lid == 170))) { return true; }
  // * 助動詞,*,*,*,特殊・デス,基本形,デス true
  if ((true) && ((lid == 171))) { return true; }
  // * 助動詞,*,*,*,特殊・デス,基本形,ドス true
  if ((true) && ((lid == 172))) { return true; }
  // * 助動詞,*,*,*,特殊・デス,未然形,ッス true
  if ((true) && ((lid == 180))) { return true; }
  // * 助動詞,*,*,*,不変化型,基本形,じゃン true
  if ((true) && ((lid == 36))) { return true; }
  // * 名詞,固有名詞,地域,一般 true
  if ((true) && ((lid >= 1847 && lid <= 1850))) { return true; }
  // 接頭詞,名詞接続 * false
  if (((rid >= 2518 && rid <= 2550)) && (true)) { return false; }
  // * 助動詞,*,*,*,形容詞・イ段,*,無い true
  if ((true) && ((lid == 65) || (lid == 67) || (lid == 69) || (lid == 71) || (lid == 73) || (lid == 75) || (lid == 77) || (lid == 79) || (lid == 81) || (lid == 83) || (lid == 85) || (lid == 87) || (lid == 89))) { return true; }
  // 動詞 助動詞,*,*,*,形容詞・イ段,*,無い true
  if (((rid >= 416 && rid <= 1764)) && ((lid == 65) || (lid == 67) || (lid == 69) || (lid == 71) || (lid == 73) || (lid == 75) || (lid == 77) || (lid == 79) || (lid == 81) || (lid == 83) || (lid == 85) || (lid == 87) || (lid == 89))) { return true; }
  // ^(助動詞|動詞),*,*,*,*,基本形 名詞,非自立,一般,*,*,*,(事|コト) true
  if (((rid == 30) || (rid >= 33 && rid <= 42) || (rid == 47) || (rid >= 56 && rid <= 57) || (rid >= 76 && rid <= 77) || (rid == 92) || (rid == 94) || (rid >= 97 && rid <= 98) || (rid >= 108 && rid <= 109) || (rid >= 114 && rid <= 115) || (rid == 120) || (rid == 123) || (rid == 128) || (rid == 131) || (rid >= 136 && rid <= 137) || (rid >= 145 && rid <= 148) || (rid == 159) || (rid >= 163 && rid <= 172) || (rid >= 194 && rid <= 197) || (rid == 208) || (rid >= 230 && rid <= 235) || (rid == 256) || (rid >= 446 && rid <= 451) || (rid >= 494 && rid <= 501) || (rid >= 540 && rid <= 542) || (rid == 567) || (rid >= 583 && rid <= 584) || (rid == 595) || (rid == 608) || (rid == 617) || (rid >= 656 && rid <= 661) || (rid == 686) || (rid == 692) || (rid == 697) || (rid == 706) || (rid == 715) || (rid >= 743 && rid <= 746) || (rid == 772) || (rid == 781) || (rid >= 799 && rid <= 802) || (rid == 824) || (rid == 832) || (rid == 840) || (rid == 851) || (rid == 861) || (rid >= 1040 && rid <= 1074) || (rid == 1183) || (rid >= 1190 && rid <= 1191) || (rid >= 1228 && rid <= 1239) || (rid >= 1304 && rid <= 1308) || (rid >= 1335 && rid <= 1336) || (rid >= 1367 && rid <= 1374) || (rid >= 1405 && rid <= 1406) || (rid >= 1509 && rid <= 1527) || (rid >= 1649 && rid <= 1653) || (rid >= 1703 && rid <= 1714) || (rid == 1762)) && ((lid == 2004) || (lid == 2012))) { return true; }
  // ^名詞 助動詞,*,*,*,(文語・ル|文語・リ|文語・マジ|文語・ゴトシ|文語・ケリ|文語・キ) true
  if (((rid >= 1765 && rid <= 2105)) && ((lid >= 90 && lid <= 100) || (lid >= 118 && lid <= 130))) { return true; }
  // ^助詞 名詞,非自立,副詞可能,*,*,*,以内 true
  if (((rid >= 259 && rid <= 415)) && ((lid == 2072))) { return true; }
  // ^(動詞|助動詞|形容詞) 名詞,非自立,副詞可能,*,*,*,他 true
  if (((rid >= 27 && rid <= 258) || (rid >= 416 && rid <= 1764) || (rid >= 2106 && rid <= 2511)) && ((lid == 2070))) { return true; }
  // ^名詞,サ変接続 動詞,自立,*,*,一段,*,できる false
  if (((rid >= 1765 && rid <= 1771)) && ((lid == 623) || (lid == 629) || (lid == 635) || (lid == 641) || (lid == 647) || (lid == 653) || (lid == 659) || (lid == 665) || (lid == 671) || (lid == 677))) { return false; }
  // * 名詞,非自立,助動詞語幹,*,*,*,よう false
  if ((true) && ((lid == 2102))) { return false; }
  // * 名詞,非自立,助動詞語幹,*,*,*,様 true
  if ((true) && ((lid == 2103))) { return true; }
  // ^名詞,非自立 ^助詞 false
  if (((rid >= 1965 && rid <= 2105)) && ((lid >= 259 && lid <= 415))) { return false; }
  // ^名詞 助詞,終助詞,*,*,*,*,(よ|ね|の) false
  if (((rid >= 1765 && rid <= 2105)) && ((lid >= 404 && lid <= 406) || (lid == 411))) { return false; }
  // ^名詞 ^助詞,(終助詞|特殊|接続助詞|動詞非自立的) true
  if (((rid >= 1765 && rid <= 2105)) && ((lid >= 320 && lid <= 355) || (lid >= 385 && lid <= 414))) { return true; }
  // ^名詞 ^名詞,非自立,(一般|助動詞語幹|形容動詞語幹) true
  if (((rid >= 1765 && rid <= 2105)) && ((lid >= 1967 && lid <= 2037) || (lid >= 2100 && lid <= 2105))) { return true; }
  // ^名詞 ^形容詞,非自立 true
  if (((rid >= 1765 && rid <= 2105)) && ((lid >= 2392 && lid <= 2511))) { return true; }
  // ^名詞 ^動詞,(接尾|非自立) true
  if (((rid >= 1765 && rid <= 2105)) && ((lid >= 416 && lid <= 558) || (lid >= 845 && lid <= 1764))) { return true; }
  // ^(動詞|形容詞|助動詞) 名詞,接尾,(サ変接続|人名|副詞可能|助数詞|地域|形容動詞語幹|形容動詞語幹) true
  if (((rid >= 27 && rid <= 258) || (rid >= 416 && rid <= 1764) || (rid >= 2106 && rid <= 2511)) && ((lid >= 1859 && lid <= 1865) || (lid >= 1912 && lid <= 1920) || (lid >= 1923 && lid <= 1940))) { return true; }
  // ^(動詞|形容詞|助動詞) 形容詞,非自立,*,*,形容詞・アウオ段,*,良い true
  if (((rid >= 27 && rid <= 258) || (rid >= 416 && rid <= 1764) || (rid >= 2106 && rid <= 2511)) && ((lid == 2401) || (lid == 2408) || (lid == 2415) || (lid == 2422) || (lid == 2429) || (lid == 2436) || (lid == 2443) || (lid == 2451) || (lid == 2458) || (lid == 2465) || (lid == 2472) || (lid == 2479) || (lid == 2486))) { return true; }
  // ^接頭詞,名詞接続 ^(動詞|形容詞) true
  if (((rid >= 2518 && rid <= 2550)) && ((lid >= 416 && lid <= 1764) || (lid >= 2106 && lid <= 2511))) { return true; }
  // 名詞,固有名詞,人名 名詞,接尾,人名 false
  if (((rid >= 1844 && rid <= 1846)) && ((lid == 1912))) { return false; }
  // 名詞,固有名詞,一般 名詞,接尾,人名 false
  if (((rid == 1843)) && ((lid == 1912))) { return false; }
  // 名詞,固有名詞,地域 名詞,接尾,地域 false
  if (((rid >= 1847 && rid <= 1851)) && ((lid >= 1928 && lid <= 1936))) { return false; }
  // 名詞,数 名詞,接尾,助数詞 true
  if (((rid >= 1951 && rid <= 1963)) && ((lid >= 1923 && lid <= 1927))) { return true; }
  // 名詞,接尾,特殊 名詞,接尾,助動詞語幹 false
  if (((rid >= 1941 && rid <= 1947)) && ((lid >= 1921 && lid <= 1922))) { return false; }
  // 名詞 名詞,接尾,副詞可能 true
  if (((rid >= 1765 && rid <= 2105)) && ((lid >= 1913 && lid <= 1920))) { return true; }
  // ^名詞,接尾 ^名詞,接尾 true
  if (((rid >= 1859 && rid <= 1949)) && ((lid >= 1859 && lid <= 1949))) { return true; }
  // * ^名詞,接尾,(副詞可能|助動詞語幹|特殊) false
  if ((true) && ((lid >= 1913 && lid <= 1922) || (lid >= 1941 && lid <= 1947))) { return false; }
  // * ^名詞,接尾 true
  if ((true) && ((lid >= 1859 && lid <= 1949))) { return true; }
  // ^助動詞,*,*,*,特殊・ダ ^名詞,非自立 false
  if (((rid >= 156 && rid <= 162)) && ((lid >= 1965 && lid <= 2105))) { return false; }
  // ^動詞 動詞,非自立,*,*,五段・ワ行促音便,*,(ちまう|ちゃう|じまう|じゃう) false
  if (((rid >= 416 && rid <= 1764)) && ((lid >= 1684 && lid <= 1685) || (lid >= 1687 && lid <= 1688) || (lid >= 1695 && lid <= 1698) || (lid >= 1705 && lid <= 1706) || (lid >= 1708 && lid <= 1709) || (lid >= 1717 && lid <= 1718) || (lid >= 1720 && lid <= 1721) || (lid >= 1728 && lid <= 1729) || (lid >= 1731 && lid <= 1732) || (lid >= 1739 && lid <= 1740) || (lid >= 1742 && lid <= 1743) || (lid >= 1751 && lid <= 1752) || (lid >= 1754 && lid <= 1755))) { return false; }
  // ^動詞 動詞,非自立,*,*,五段・カ行促音便,*,(てく|どく) false
  if (((rid >= 416 && rid <= 1764)) && ((lid == 1290) || (lid == 1295) || (lid == 1301) || (lid == 1306) || (lid == 1311) || (lid == 1316) || (lid == 1321) || (lid == 1326))) { return false; }
  // ^動詞 動詞,非自立,*,*,五段・カ行イ音便,*,とく false
  if (((rid >= 416 && rid <= 1764)) && ((lid == 1194) || (lid == 1206) || (lid == 1218) || (lid == 1230) || (lid == 1242) || (lid == 1254) || (lid == 1266) || (lid == 1278))) { return false; }
  // ^動詞 動詞,非自立,*,*,一段,*,(てる|でる) false
  if (((rid >= 416 && rid <= 1764)) && ((lid == 879) || (lid == 881) || (lid == 914) || (lid == 916) || (lid == 949) || (lid == 951) || (lid == 984) || (lid == 986) || (lid == 1019) || (lid == 1021) || (lid == 1054) || (lid == 1056) || (lid == 1089) || (lid == 1091) || (lid == 1124) || (lid == 1126) || (lid == 1158) || (lid == 1160))) { return false; }
  // ^動詞 動詞,非自立,*,*,五段・ラ行,*,(とる|どる) false
  if (((rid >= 416 && rid <= 1764)) && ((lid >= 1420 && lid <= 1421) || (lid >= 1439 && lid <= 1440) || (lid >= 1459 && lid <= 1460) || (lid >= 1478 && lid <= 1479) || (lid >= 1495 && lid <= 1496) || (lid >= 1514 && lid <= 1515) || (lid >= 1533 && lid <= 1534) || (lid >= 1552 && lid <= 1553) || (lid >= 1571 && lid <= 1572) || (lid >= 1590 && lid <= 1591) || (lid >= 1609 && lid <= 1610))) { return false; }
  // ^副詞,助詞類接続,*,*,*,*,(そう|こう|どう|どぉ) ^動詞,自立,*,*,五段・ラ行,*,なる false
  if (((rid == 17) || (rid >= 21 && rid <= 23)) && ((lid == 721) || (lid == 725) || (lid == 729) || (lid == 733) || (lid == 737) || (lid == 741) || (lid == 745) || (lid == 749) || (lid == 753) || (lid == 757) || (lid == 761) || (lid == 765))) { return false; }
  // ^形容詞,自立,*,*,*,連用テ接続 ^動詞,自立,*,*,五段・ラ行,*,なる false
  if (((rid >= 2373 && rid <= 2377) || (rid == 2391)) && ((lid == 721) || (lid == 725) || (lid == 729) || (lid == 733) || (lid == 737) || (lid == 741) || (lid == 745) || (lid == 749) || (lid == 753) || (lid == 757) || (lid == 761) || (lid == 765))) { return false; }
  // ^名詞,副詞可能 ^動詞,自立,*,*,五段・ラ行,*,なる false
  if (((rid >= 1832 && rid <= 1841)) && ((lid == 721) || (lid == 725) || (lid == 729) || (lid == 733) || (lid == 737) || (lid == 741) || (lid == 745) || (lid == 749) || (lid == 753) || (lid == 757) || (lid == 761) || (lid == 765))) { return false; }
  // ^助詞,*,*,*,*,*,(は|に|で|も|が|の) ^動詞,自立,*,*,五段・ラ行,*,ある true
  if (((rid >= 274 && rid <= 275) || (rid == 290) || (rid == 299) || (rid >= 318 && rid <= 319) || (rid >= 323 && rid <= 324) || (rid == 337) || (rid >= 345 && rid <= 348) || (rid == 350) || (rid == 355) || (rid >= 357 && rid <= 358) || (rid >= 360 && rid <= 362) || (rid == 364) || (rid >= 383 && rid <= 384) || (rid >= 387 && rid <= 388) || (rid == 402) || (rid >= 405 && rid <= 406) || (rid == 408) || (rid == 415)) && ((lid == 719) || (lid == 723) || (lid == 727) || (lid == 731) || (lid == 735) || (lid == 739) || (lid == 743) || (lid == 747) || (lid == 751) || (lid == 755) || (lid == 759) || (lid == 763))) { return true; }
  // ^助詞,*,*,*,*,*,(は|に|で|も|が|の|と) ^動詞,自立,*,*,サ変・スル,* false
  if (((rid >= 262 && rid <= 263) || (rid >= 274 && rid <= 275) || (rid >= 290 && rid <= 291) || (rid == 299) || (rid >= 317 && rid <= 319) || (rid >= 323 && rid <= 324) || (rid >= 337 && rid <= 339) || (rid >= 345 && rid <= 348) || (rid == 350) || (rid == 355) || (rid >= 357 && rid <= 362) || (rid == 364) || (rid == 375) || (rid == 377) || (rid >= 383 && rid <= 384) || (rid >= 387 && rid <= 388) || (rid == 402) || (rid >= 405 && rid <= 406) || (rid == 408) || (rid == 415)) && ((lid >= 601 && lid <= 613))) { return false; }
  // ^名詞,(一般|サ変接続|形容動詞語幹|副詞可能) ^動詞,自立,*,*,サ変・スル,* false
  if (((rid >= 1765 && rid <= 1771) || (rid >= 1773 && rid <= 1822) || (rid >= 1832 && rid <= 1841) || (rid >= 1854 && rid <= 1858)) && ((lid >= 601 && lid <= 613))) { return false; }
  // ^(形容詞|動詞|副詞) ^動詞,自立,*,*,サ変・スル,* false
  if (((rid >= 12 && rid <= 26) || (rid >= 416 && rid <= 1764) || (rid >= 2106 && rid <= 2511)) && ((lid >= 601 && lid <= 613))) { return false; }
  // * ^動詞,自立,*,*,サ変・スル,* true
  if ((true) && ((lid >= 601 && lid <= 613))) { return true; }
  // ^助詞,*,*,*,*,*,(は|に|で|も|が|の) 形容詞,自立,*,*,形容詞・アウオ段,*,ない false
  if (((rid >= 274 && rid <= 275) || (rid == 290) || (rid == 299) || (rid >= 318 && rid <= 319) || (rid >= 323 && rid <= 324) || (rid == 337) || (rid >= 345 && rid <= 348) || (rid == 350) || (rid == 355) || (rid >= 357 && rid <= 358) || (rid >= 360 && rid <= 362) || (rid == 364) || (rid >= 383 && rid <= 384) || (rid >= 387 && rid <= 388) || (rid == 402) || (rid >= 405 && rid <= 406) || (rid == 408) || (rid == 415)) && ((lid == 2305) || (lid == 2310) || (lid == 2315) || (lid == 2320) || (lid == 2325) || (lid == 2330) || (lid == 2335) || (lid == 2340) || (lid == 2345) || (lid == 2349) || (lid == 2354) || (lid == 2359) || (lid == 2364) || (lid == 2369) || (lid == 2374))) { return false; }
  // ^助詞,格助詞,引用,*,*,*,と ^動詞,自立,*,*,五段・ワ行促音便,*,いう false
  if (((rid == 375)) && ((lid == 787) || (lid == 791) || (lid == 796) || (lid == 800) || (lid == 804) || (lid == 808) || (lid == 812) || (lid == 816))) { return false; }
  // * 名詞,非自立,形容動詞語幹,*,*,*,ふう true
  if ((true) && ((lid == 2104))) { return true; }
  // ^副詞,一般 形容詞,(接尾|非自立) true
  if (((rid >= 12 && rid <= 15)) && ((lid >= 2106 && lid <= 2301) || (lid >= 2392 && lid <= 2511))) { return true; }
  // ^助詞 形容詞,(接尾|非自立) true
  if (((rid >= 259 && rid <= 415)) && ((lid >= 2106 && lid <= 2301) || (lid >= 2392 && lid <= 2511))) { return true; }
  // 助動詞,*,*,*,特殊・ダ 名詞 true
  if (((rid >= 156 && rid <= 162)) && ((lid >= 1765 && lid <= 2105))) { return true; }
  // * ^(その他|フィラー|感動詞) true
  if ((true) && ((lid >= 1 && lid <= 11) || (lid == 2512))) { return true; }
  // * ^記号,(括弧開|括弧閉|アルファベット|一般) true
  if ((true) && ((lid >= 2553 && lid <= 2554) || (lid >= 2556 && lid <= 2557))) { return true; }
  // * ^記号,(句点|読点) true
  if ((true) && ((lid == 2555) || (lid == 2558))) { return true; }
  // * ^形容詞,自立 true
  if ((true) && ((lid >= 2302 && lid <= 2391))) { return true; }
  // * ^形容詞,(接尾|非自立) false
  if ((true) && ((lid >= 2106 && lid <= 2301) || (lid >= 2392 && lid <= 2511))) { return false; }
  // * ^(助詞|助動詞) false
  if ((true) && ((lid >= 27 && lid <= 415))) { return false; }
  // * ^動詞,自立 true
  if ((true) && ((lid >= 559 && lid <= 844))) { return true; }
  // * ^動詞,接尾 false
  if ((true) && ((lid >= 416 && lid <= 558))) { return false; }
  // * ^名詞,(接続詞的|接尾|動詞非自立的|特殊|非自立) false
  if ((true) && ((lid == 1842) || (lid >= 1859 && lid <= 1950) || (lid >= 1964 && lid <= 2105))) { return false; }
  // * ^名詞 true
  if ((true) && ((lid >= 1765 && lid <= 2105))) { return true; }
  // * ^助詞,接続助詞,*,*,*,*,て false
  if ((true) && ((lid == 336))) { return false; }
  // * 動詞,非自立,*,*,*,*,(いける|いる|える|かける|かねる|きれる|すぎる|たげる|つづける|づける|づける|できる|どける|のける|みせる|みる|もらえる|る|く|尽くす) false
  if ((true) && ((lid >= 845 && lid <= 854) || (lid >= 866 && lid <= 872) || (lid == 875) || (lid >= 877 && lid <= 878) || (lid == 880) || (lid == 882) || (lid >= 884 && lid <= 887) || (lid >= 901 && lid <= 907) || (lid == 910) || (lid >= 912 && lid <= 913) || (lid == 915) || (lid == 917) || (lid >= 919 && lid <= 922) || (lid >= 936 && lid <= 942) || (lid == 945) || (lid >= 947 && lid <= 948) || (lid == 950) || (lid == 952) || (lid >= 954 && lid <= 957) || (lid >= 971 && lid <= 977) || (lid == 980) || (lid >= 982 && lid <= 983) || (lid == 985) || (lid == 987) || (lid >= 989 && lid <= 992) || (lid >= 1006 && lid <= 1012) || (lid == 1015) || (lid >= 1017 && lid <= 1018) || (lid == 1020) || (lid == 1022) || (lid >= 1024 && lid <= 1027) || (lid >= 1041 && lid <= 1047) || (lid == 1050) || (lid >= 1052 && lid <= 1053) || (lid == 1055) || (lid == 1057) || (lid >= 1059 && lid <= 1062) || (lid >= 1076 && lid <= 1082) || (lid == 1085) || (lid >= 1087 && lid <= 1088) || (lid == 1090) || (lid == 1092) || (lid >= 1094 && lid <= 1097) || (lid >= 1111 && lid <= 1117) || (lid == 1120) || (lid >= 1122 && lid <= 1123) || (lid == 1125) || (lid == 1127) || (lid >= 1129 && lid <= 1131) || (lid >= 1145 && lid <= 1151) || (lid == 1154) || (lid >= 1156 && lid <= 1157) || (lid == 1159) || (lid == 1161) || (lid >= 1163 && lid <= 1165) || (lid >= 1178 && lid <= 1187) || (lid == 1289) || (lid == 1294) || (lid == 1300) || (lid == 1305) || (lid == 1310) || (lid == 1315) || (lid == 1320) || (lid == 1325) || (lid == 1348) || (lid == 1356) || (lid == 1364) || (lid == 1372) || (lid == 1380) || (lid == 1388) || (lid == 1396) || (lid == 1625) || (lid == 1630) || (lid == 1635) || (lid == 1640) || (lid == 1650) || (lid == 1655) || (lid == 1660) || (lid == 1665) || (lid == 1670) || (lid == 1675))) { return false; }
  // * 助詞,接続助詞,*,*,*,*,で false
  if ((true) && ((lid == 337))) { return false; }
  // * * true
  if ((true) && (true)) { return true; }
  return true;  // default
}
}   // namespace

