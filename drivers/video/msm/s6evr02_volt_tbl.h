#ifndef __REF_VOLT_TABLE_H__
#define __REF_VOLT_TABLE_H__

u32 volt_table_vt[16] = {
	399769600,	394199040,	388628480,	383057920,
	377487360,	371851264,	366280704,	360710144,
	355139584,	349569024,	335609856,	330956800,
	326303744,	321650688,	316997632,	313327616,
};

u32 volt_table_v255[505] = {
	366300517,	365835669,	365370820,	364905972,	364441124,	363976275,	363511427,	363046579,
	362581730,	362116882,	361652033,	361187185,	360722337,	360257488,	359792640,	359327792,
	358862943,	358398095,	357933247,	357468398,	357003550,	356538701,	356073853,	355609005,
	355144156,	354679308,	354214460,	353749611,	353284763,	352819914,	352355066,	351890218,
	351425369,	350960521,	350495673,	350030824,	349565976,	349101127,	348636279,	348171431,
	347706582,	347241734,	346776886,	346312037,	345847189,	345382340,	344917492,	344452644,
	343987795,	343522947,	343058099,	342593250,	342128402,	341663553,	341198705,	340733857,
	340269008,	339804160,	339339312,	338874463,	338409615,	337944767,	337479918,	337015070,
	336550221,	336085373,	335620525,	335155676,	334690828,	334225980,	333761131,	333296283,
	332831434,	332366586,	331901738,	331436889,	330972041,	330507193,	330042344,	329577496,
	329112647,	328647799,	328182951,	327718102,	327253254,	326788406,	326323557,	325858709,
	325393860,	324929012,	324464164,	323999315,	323534467,	323069619,	322604770,	322139922,
	321675073,	321210225,	320745377,	320280528,	319815680,	319350832,	318885983,	318421135,
	317956287,	317491438,	317026590,	316561741,	316096893,	315632045,	315167196,	314702348,
	314237500,	313772651,	313307803,	312842954,	312378106,	311913258,	311448409,	310983561,
	310518713,	310053864,	309589016,	309124167,	308659319,	308194471,	307729622,	307264774,
	306799926,	306335077,	305870229,	305405380,	304940532,	304475684,	304010835,	303545987,
	303081139,	302616290,	302151442,	301686593,	301221745,	300756897,	300292048,	299827200,
	299362352,	298897503,	298432655,	297967807,	297502958,	297038110,	296573261,	296108413,
	295643565,	295178716,	294713868,	294249020,	293784171,	293319323,	292854474,	292389626,
	291924778,	291459929,	290995081,	290530233,	290065384,	289600536,	289135687,	288670839,
	288205991,	287741142,	287276294,	286811446,	286346597,	285881749,	285416900,	284952052,
	284487204,	284022355,	283557507,	283092659,	282627810,	282162962,	281698113,	281233265,
	280768417,	280303568,	279838720,	279373872,	278909023,	278444175,	277979327,	277514478,
	277049630,	276584781,	276119933,	275655085,	275190236,	274725388,	274260540,	273795691,
	273330843,	272865994,	272401146,	271936298,	271471449,	271006601,	270541753,	270076904,
	269612056,	269147207,	268682359,	268217511,	267752662,	267287814,	266822966,	266358117,
	265893269,	265428420,	264963572,	264498724,	264033875,	263569027,	263104179,	262639330,
	262174482,	261709633,	261244785,	260779937,	260315088,	259850240,	259385392,	258920543,
	258455695,	257990847,	257525998,	257061150,	256596301,	256131453,	255666605,	255201756,
	254736908,	254272060,	253807211,	253342363,	252877514,	252412666,	251947818,	251482969,
	251018121,	250553273,	250088424,	249623576,	249158727,	248693879,	248229031,	247764182,
	247299334,	246834486,	246369637,	245904789,	245439940,	244975092,	244510244,	244045395,
	243580547,	243115699,	242650850,	242186002,	241721153,	241256305,	240791457,	240326608,
	239861760,	239396912,	238932063,	238467215,	238002367,	237537518,	237072670,	236607821,
	236142973,	235678125,	235213276,	234748428,	234283580,	233818731,	233353883,	232889034,
	232424186,	231959338,	231494489,	231029641,	230564793,	230099944,	229635096,	229170247,
	228705399,	228240551,	227775702,	227310854,	226846006,	226381157,	225916309,	225451460,
	224986612,	224521764,	224056915,	223592067,	223127219,	222662370,	222197522,	221732673,
	221267825,	220802977,	220338128,	219873280,	219408432,	218943583,	218478735,	218013887,
	217549038,	217084190,	216619341,	216154493,	215689645,	215224796,	214759948,	214295100,
	213830251,	213365403,	212900554,	212435706,	211970858,	211506009,	211041161,	210576313,
	210111464,	209646616,	209181767,	208716919,	208252071,	207787222,	207322374,	206857526,
	206392677,	205927829,	205462980,	204998132,	204533284,	204068435,	203603587,	203138739,
	202673890,	202209042,	201744193,	201279345,	200814497,	200349648,	199884800,	199419952,
	198955103,	198490255,	198025407,	197560558,	197095710,	196630861,	196166013,	195701165,
	195236316,	194771468,	194306620,	193841771,	193376923,	192912074,	192447226,	191982378,
	191517529,	191052681,	190587833,	190122984,	189658136,	189193287,	188728439,	188263591,
	187798742,	187333894,	186869046,	186404197,	185939349,	185474500,	185009652,	184544804,
	184079955,	183615107,	183150259,	182685410,	182220562,	181755713,	181290865,	180826017,
	180361168,	179896320,	179431472,	178966623,	178501775,	178036927,	177572078,	177107230,
	176642381,	176177533,	175712685,	175247836,	174782988,	174318140,	173853291,	173388443,
	172923594,	172458746,	171993898,	171529049,	171064201,	170599353,	170134504,	169669656,
	169204807,	168739959,	168275111,	167810262,	167345414,	166880566,	166415717,	165950869,
	165486020,	165021172,	164556324,	164091475,	163626627,	163161779,	162696930,	162232082,
	161767233,	161302385,	160837537,	160372688,	159907840,	159442992,	158978143,	158513295,
	158048447,	157583598,	157118750,	156653901,	156189053,	155724205,	155259356,	154794508,
	154329660,	153864811,	153399963,	152935114,	152470266,	152005418,	151540569,	151075721,
	150610873,	150146024,	149681176,	149216327,	148751479,	148286631,	147821782,	147356934,
	146892086,	146427237,	145962389,	145497540,	145032692,	144567844,	144102995,	143638147,
	143173299,	142708450,	142243602,	141778753,	141313905,	140849057,	140384208,	139919360,
	139454512,	138989663,	138524815,	138059967,	137595118,	137130270,	136665421,	136200573,
	135735725,	135270876,	134806028,	134341180,	133876331,	133411483,	132946634,	132481786,
	132016938,
};

u32 volt_table_cv_64_dv_320[256] = {
	13104,	13308,	13513,	13718,	13923,	14127,	14332,	14537,
	14742,	14946,	15151,	15356,	15561,	15765,	15970,	16175,
	16380,	16585,	16789,	16994,	17199,	17404,	17608,	17813,
	18018,	18223,	18427,	18632,	18837,	19042,	19246,	19451,
	19656,	19861,	20065,	20270,	20475,	20680,	20884,	21089,
	21294,	21499,	21703,	21908,	22113,	22318,	22522,	22727,
	22932,	23137,	23341,	23546,	23751,	23956,	24160,	24365,
	24570,	24775,	24979,	25184,	25389,	25594,	25798,	26003,
	26208,	26413,	26617,	26822,	27027,	27232,	27436,	27641,
	27846,	28051,	28255,	28460,	28665,	28870,	29074,	29279,
	29484,	29689,	29893,	30098,	30303,	30508,	30712,	30917,
	31122,	31327,	31531,	31736,	31941,	32146,	32350,	32555,
	32760,	32965,	33170,	33374,	33579,	33784,	33989,	34193,
	34398,	34603,	34808,	35012,	35217,	35422,	35627,	35831,
	36036,	36241,	36446,	36650,	36855,	37060,	37265,	37469,
	37674,	37879,	38084,	38288,	38493,	38698,	38903,	39107,
	39312,	39517,	39722,	39926,	40131,	40336,	40541,	40745,
	40950,	41155,	41360,	41564,	41769,	41974,	42179,	42383,
	42588,	42793,	42998,	43202,	43407,	43612,	43817,	44021,
	44226,	44431,	44636,	44840,	45045,	45250,	45455,	45659,
	45864,	46069,	46274,	46478,	46683,	46888,	47093,	47297,
	47502,	47707,	47912,	48116,	48321,	48526,	48731,	48935,
	49140,	49345,	49550,	49755,	49959,	50164,	50369,	50574,
	50778,	50983,	51188,	51393,	51597,	51802,	52007,	52212,
	52416,	52621,	52826,	53031,	53235,	53440,	53645,	53850,
	54054,	54259,	54464,	54669,	54873,	55078,	55283,	55488,
	55692,	55897,	56102,	56307,	56511,	56716,	56921,	57126,
	57330,	57535,	57740,	57945,	58149,	58354,	58559,	58764,
	58968,	59173,	59378,	59583,	59787,	59992,	60197,	60402,
	60606,	60811,	61016,	61221,	61425,	61630,	61835,	62040,
	62244,	62449,	62654,	62859,	63063,	63268,	63473,	63678,
	63882,	64087,	64292,	64497,	64701,	64906,	65111,	65316
};

const u32 gamma_300_gra_table[256] = {
	0,	2,	7,	17,	32,	53,	78, 110,
	148,	191,	241,	298,	361,	430,	506,	589,
	679,	776,	880,	991,	1109,	1235,	1368,	1508,
	1657,	1812,	1975,	2147,	2325,	2512,	2706,	2909,
	3119,	3338,	3564,	3799,	4042,	4293,	4553,	4820,
	5096,	5381,	5674,	5975,	6285,	6604,	6931,	7267,
	7611,	7965,	8327,	8697,	9077,	9465,	9863,	10269,
	10684,	11109,	11542,	11984,	12436,	12896,	13366,	13845,
	14333,	14830,	15337,	15852,	16378,	16912,	17456,	18009,
	18572,	19144,	19726,	20317,	20918,	21528,	22148,	22778,
	23417,	24066,	24724,	25392,	26070,	26758,	27456,	28163,
	28880,	29607,	30344,	31090,	31847,	32613,	33390,	34176,
	34973,	35779,	36596,	37422,	38259,	39106,	39963,	40830,
	41707,	42594,	43492,	44399,	45317,	46246,	47184,	48133,
	49092,	50062,	51042,	52032,	53032,	54043,	55065,	56097,
	57139,	58192,	59255,	60329,	61413,	62508,	63613,	64729,
	65856,	66993,	68141,	69299,	70469,	71648,	72839,	74040,
	75252,	76475,	77708,	78952,	80207,	81473,	82750,	84037,
	85336,	86645,	87965,	89296,	90638,	91990,	93354,	94729,
	96114,	97511,	98919,	100337,	101767,	103208,	104659,	106122,
	107596,	109081,	110577,	112085,	113603,	115132,	116673,	118225,
	119788,	121362,	122948,	124544,	126152,	127772,	129402,	131044,
	132697,	134361,	136037,	137724,	139422,	141132,	142853,	144586,
	146330,	148085,	149852,	151630,	153419,	155220,	157033,	158857,
	160692,	162540,	164398,	166268,	168150,	170043,	171948,	173864,
	175792,	177731,	179683,	181645,	183620,	185606,	187603,	189613,
	191634,	193667,	195711,	197767,	199835,	201915,	204006,	206109,
	208224,	210351,	212489,	214640,	216802,	218976,	221161,	223359,
	225569,	227790,	230023,	232268,	234525,	236794,	239075,	241368,
	243672,	245989,	248318,	250658,	253011,	255375,	257752,	260141,
	262541,	264954,	267379,	269815,	272264,	274725,	277198,	279683,
	282180,	284689,	287211,	289744,	292290,	294848,	297418,	300000
};

const u32 gamma_control_set_21[] = {
	0,	9,	38,	89,	163,	260,	381,	526,
	697,	892,	1113,	1359,	1632,	1930,	2255,	2607,
	2985,	3391,	3823,	4283,	4770,	5284,	5826,	6396,
	6994,	7620,	8274,	8957,	9668,	10407,	11175,	11971,
	12797,	13651,	14534,	15446,	16388,	17358,	18358,	19387,
	20446,	21534,	22652,	23799,	24976,	26183,	27420,	28687,
	29983,	31310,	32667,	34054,	35471,	36919,	38397,	39905,
	41444,	43014,	44614,	46244,	47906,	49598,	51321,	53074,
	54859,	56674,	58521,	60399,	62307,	64247,	66218,	68220,
	70253,	72318,	74414,	76542,	78700,	80891,	83113,	85366,
	87651,	89968,	92316,	94696,	97108,	99551,	102027,	104534,
	107073,	109644,	112248,	114883,	117550,	120249,	122980,	125744,
	128540,	131367,	134228,	137120,	140045,	143002,	145991,	149013,
	152068,	155155,	158274,	161426,	164610,	167827,	171077,	174359,
	177674,	181022,	184403,	187816,	191262,	194741,	198253,	201797,
	205375,	208985,	212629,	216305,	220015,	223758,	227533,	231342,
	235184,	239059,	242967,	246909,	250883,	254891,	258932,	263007,
	267115,	271256,	275431,	279639,	283880,	288155,	292464,	296806,
	301181,	305590,	310033,	314509,	319019,	323562,	328139,	332750,
	337394,	342073,	346785,	351530,	356310,	361123,	365971,	370852,
	375767,	380715,	385698,	390715,	395766,	400851,	405969,	411122,
	416309,	421530,	426785,	432074,	437397,	442754,	448146,	453572,
	459032,	464526,	470054,	475617,	481214,	486845,	492511,	498211,
	503945,	509714,	515517,	521355,	527227,	533133,	539074,	545050,
	551060,	557104,	563183,	569297,	575445,	581628,	587845,	594097,
	600384,	606705,	613061,	619452,	625877,	632338,	638833,	645362,
	651927,	658526,	665161,	671830,	678533,	685272,	692046,	698854,
	705698,	712576,	719490,	726438,	733421,	740440,	747493,	754581,
	761705,	768863,	776057,	783286,	790550,	797848,	805183,	812552,
	819956,	827396,	834871,	842381,	849926,	857506,	865122,	872773,
	880460,	888181,	895938,	903731,	911558,	919421,	927320,	935254,
	943223,	951228,	959268,	967343,	975454,	983601,	991783,	1000000
};

const u32 gamma_control_set_213[] = {
	0,	8,	33,	78,	144,	231,	341,	473,
	628,	807,	1010,	1237,	1489,	1766,	2067,	2395,
	2747,	3126,	3531,	3962,	4419,	4903,	5413,	5951,
	6516,	7107,	7727,	8373,	9048,	9750,	10480,	11238,
	12024,	12839,	13682,	14553,	15453,	16381,	17339,	18325,
	19341,	20385,	21459,	22561,	23694,	24855,	26047,	27268,
	28518,	29798,	31109,	32449,	33819,	35219,	36650,	38111,
	39602,	41123,	42675,	44258,	45871,	47515,	49189,	50894,
	52630,	54397,	56196,	58025,	59885,	61776,	63699,	65653,
	67638,	69655,	71703,	73782,	75894,	78036,	80211,	82417,
	84655,	86925,	89227,	91560,	93926,	96324,	98754,	101216,
	103710,	106236,	108795,	111386,	114009,	116665,	119353,	122074,
	124827,	127613,	130432,	133283,	136167,	139083,	142033,	145015,
	148031,	151079,	154160,	157274,	160422,	163602,	166816,	170063,
	173343,	176656,	180002,	183382,	186795,	190242,	193722,	197236,
	200783,	204363,	207978,	211626,	215307,	219023,	222772,	226554,
	230371,	234221,	238106,	242024,	245976,	249962,	253982,	258037,
	262125,	266247,	270404,	274594,	278819,	283079,	287372,	291700,
	296062,	300458,	304889,	309354,	313854,	318388,	322957,	327560,
	332198,	336870,	341577,	346319,	351096,	355907,	360753,	365633,
	370549,	375499,	380484,	385505,	390560,	395650,	400775,	405935,
	411130,	416360,	421625,	426925,	432260,	437631,	443037,	448478,
	453954,	459466,	465012,	470594,	476212,	481865,	487553,	493277,
	499036,	504831,	510661,	516526,	522428,	528364,	534337,	540345,
	546388,	552468,	558583,	564733,	570920,	577142,	583400,	589694,
	596024,	602389,	608791,	615228,	621702,	628211,	634756,	641337,
	647955,	654608,	661297,	668023,	674785,	681582,	688416,	695286,
	702193,	709135,	716114,	723129,	730180,	737268,	744392,	751552,
	758749,	765982,	773251,	780557,	787900,	795279,	802694,	810146,
	817634,	825159,	832721,	840319,	847954,	855625,	863333,	871078,
	878860,	886678,	894533,	902425,	910353,	918319,	926321,	934360,
	942436,	950548,	958698,	966885,	975108,	983369,	991666,	1000000
};

const u32 gamma_control_set_215[] = {
	0,	7,	30,	72, 132,	214,	316,	440,
	586,	755,	947,	1162,	1401,	1664,	1951,	2263,
	2599,	2961,	3348,	3761,	4200,	4664,	5155,	5671,
	6215,	6785,	7382,	8006,	8657,	9335,	10041,	10774,
	11535,	12324,	13141,	13986,	14859,	15761,	16691,	17650,
	18637,	19653,	20698,	21772,	22876,	24008,	25170,	26361,
	27581,	28832,	30111,	31421,	32761,	34130,	35530,	36959,
	38419,	39909,	41430,	42981,	44562,	46175,	47817,	49491,
	51195,	52931,	54697,	56494,	58323,	60182,	62073,	63995,
	65949,	67934,	69950,	71998,	74078,	76190,	78333,	80508,
	82715,	84954,	87225,	89528,	91863,	94231,	96630,	99062,
	101526,	104023,	106552,	109114,	111708,	114335,	116994,	119687,
	122412,	125170,	127961,	130784,	133641,	136531,	139454,	142410,
	145399,	148422,	151477,	154566,	157689,	160845,	164034,	167257,
	170513,	173803,	177127,	180485,	183876,	187301,	190759,	194252,
	197779,	201339,	204934,	208562,	212225,	215922,	219653,	223418,
	227217,	231051,	234919,	238821,	242758,	246729,	250735,	254775,
	258850,	262959,	267103,	271282,	275496,	279744,	284027,	288345,
	292697,	297085,	301507,	305965,	310457,	314985,	319548,	324145,
	328778,	333446,	338150,	342888,	347662,	352471,	357316,	362195,
	367111,	372062,	377048,	382070,	387127,	392220,	397348,	402513,
	407713,	412948,	418220,	423527,	428870,	434248,	439663,	445114,
	450600,	456123,	461681,	467276,	472906,	478573,	484276,	490014,
	495790,	501601,	507448,	513332,	519252,	525209,	531201,	537231,
	543296,	549398,	555537,	561712,	567923,	574171,	580456,	586777,
	593135,	599529,	605961,	612429,	618933,	625475,	632053,	638668,
	645320,	652009,	658735,	665497,	672297,	679133,	686007,	692918,
	699865,	706850,	713872,	720931,	728027,	735161,	742331,	749539,
	756784,	764067,	771387,	778744,	786138,	793570,	801039,	808546,
	816090,	823672,	831291,	838947,	846642,	854374,	862143,	869950,
	877795,	885677,	893597,	901555,	909551,	917584,	925655,	933764,
	941911,	950096,	958318,	966579,	974877,	983214,	991588,	1000000
};

const u32 gamma_control_set_218[] = {
	0	,	6,	26,	63,	117,	190,	282,	395,
	528,	683,	859,	1057,	1278,	1522,	1788,	2078,
	2392,	2730,	3092,	3479,	3891,	4327,	4789,	5277,
	5789,	6328,	6893,	7484,	8102,	8746,	9417,	10114,
	10839,	11591,	12370,	13177,	14012,	14874,	15765,	16683,
	17630,	18605,	19608,	20640,	21701,	22791,	23909,	25057,
	26234,	27440,	28675,	29940,	31235,	32559,	33913,	35297,
	36711,	38155,	39630,	41134,	42669,	44235,	45831,	47458,
	49116,	50804,	52523,	54274,	56055,	57868,	59712,	61587,
	63494,	65432,	67402,	69403,	71436,	73501,	75598,	77727,
	79888,	82081,	84306,	86564,	88853,	91175,	93530,	95917,
	98337,	100789,	103275,	105792,	108343,	110927,	113544,	116193,
	118876,	121592,	124342,	127124,	129940,	132790,	135673,	138589,
	141539,	144523,	147540,	150592,	153677,	156795,	159948,	163135,
	166356,	169611,	172900,	176224,	179582,	182974,	186400,	189861,
	193356,	196886,	200451,	204050,	207684,	211352,	215056,	218794,
	222567,	226375,	230218,	234096,	238010,	241958,	245941,	249960,
	254014,	258104,	262228,	266389,	270584,	274815,	279082,	283384,
	287722,	292096,	296505,	300950,	305431,	309948,	314501,	319090,
	323714,	328375,	333072,	337805,	342574,	347380,	352221,	357099,
	362013,	366964,	371951,	376974,	382034,	387131,	392264,	397434,
	402640,	407883,	413163,	418480,	423833,	429223,	434651,	440115,
	445616,	451154,	456729,	462341,	467990,	473677,	479401,	485161,
	490960,	496795,	502668,	508578,	514526,	520511,	526533,	532593,
	538691,	544826,	550999,	557209,	563457,	569743,	576067,	582428,
	588828,	595265,	601740,	608253,	614804,	621393,	628020,	634685,
	641388,	648129,	654909,	661726,	668582,	675477,	682409,	689380,
	696389,	703437,	710523,	717647,	724810,	732011,	739251,	746530,
	753847,	761203,	768598,	776031,	783503,	791014,	798563,	806152,
	813779,	821445,	829150,	836894,	844677,	852499,	860360,	868261,
	876200,	884178,	892196,	900252,	908348,	916484,	924658,	932872,
	941125,	949417,	957749,	966121,	974531,	982982,	991471,	1000000
};

const u32 gamma_control_set_22[] = {
	0,	6,	24,	57,	108,	176,	262,	368,
	493,	639,	805,	993,	1202,	1434,	1687,	1964,
	2263,	2586,	2933,	3303,	3698,	4117,	4560,	5029,
	5522,	6041,	6585,	7156,	7752,	8374,	9022,	9697,
	10398,	11127,	11882,	12664,	13474,	14311,	15176,	16068,
	16989,	17937,	18913,	19918,	20952,	22013,	23104,	24223,
	25372,	26549,	27756,	28992,	30257,	31552,	32876,	34231,
	35615,	37029,	38473,	39948,	41452,	42988,	44553,	46149,
	47776,	49434,	51123,	52842,	54593,	56375,	58188,	60032,
	61908,	63815,	65754,	67725,	69728,	71762,	73828,	75927,
	78057,	80220,	82415,	84642,	86902,	89194,	91519,	93876,
	96267,	98690,	101146,	103635,	106157,	108712,	111300,	113921,
	116576,	119265,	121986,	124741,	127530,	130353,	133209,	136099,
	139023,	141981,	144973,	147999,	151059,	154153,	157281,	160444,
	163641,	166873,	170139,	173440,	176775,	180145,	183549,	186989,
	190463,	193973,	197517,	201096,	204711,	208360,	212045,	215765,
	219520,	223311,	227137,	230999,	234896,	238828,	242797,	246801,
	250841,	254916,	259028,	263175,	267359,	271578,	275833,	280125,
	284453,	288816,	293217,	297653,	302126,	306635,	311181,	315763,
	320382,	325037,	329730,	334458,	339224,	344026,	348865,	353741,
	358654,	363604,	368591,	373616,	378677,	383775,	388911,	394084,
	399294,	404541,	409826,	415149,	420508,	425906,	431341,	436813,
	442323,	447871,	453457,	459080,	464742,	470441,	476178,	481953,
	487766,	493617,	499506,	505433,	511398,	517402,	523444,	529524,
	535642,	541799,	547994,	554228,	560500,	566810,	573159,	579547,
	585973,	592439,	598942,	605485,	612066,	618686,	625345,	632043,
	638780,	645556,	652371,	659224,	666117,	673050,	680021,	687031,
	694081,	701170,	708298,	715466,	722673,	729919,	737205,	744531,
	751896,	759300,	766744,	774228,	781751,	789314,	796917,	804560,
	812242,	819964,	827726,	835528,	843370,	851252,	859174,	867136,
	875138,	883180,	891263,	899385,	907548,	915751,	923994,	932277,
	940601,	948965,	957370,	965815,	974301,	982827,	991393,	1000000
};

const u32 gamma_control_set_221[] = {
	0,	5,	23,	55,	103,	169,	252,	355,
	476,	618,	780,	962,	1166,	1392,	1639,	1909,
	2202,	2517,	2856,	3219,	3605,	4015,	4450,	4909,
	5393,	5902,	6437,	6997,	7582,	8194,	8831,	9495,
	10185,	10901,	11645,	12415,	13213,	14037,	14890,	15769,
	16677,	17612,	18575,	19567,	20587,	21635,	22712,	23817,
	24952,	26115,	27307,	28529,	29780,	31060,	32370,	33710,
	35079,	36478,	37908,	39367,	40857,	42377,	43928,	45509,
	47120,	48763,	50436,	52141,	53876,	55642,	57440,	59269,
	61130,	63022,	64946,	66901,	68889,	70908,	72959,	75042,
	77157,	79305,	81485,	83697,	85942,	88219,	90530,	92872,
	95248,	97656,	100098,	102572,	105080,	107621,	110195,	112802,
	115443,	118117,	120825,	123567,	126342,	129151,	131994,	134871,
	137782,	140727,	143706,	146719,	149766,	152848,	155964,	159115,
	162300,	165520,	168775,	172064,	175388,	178747,	182141,	185569,
	189033,	192532,	196066,	199635,	203240,	206880,	210555,	214266,
	218012,	221794,	225612,	229465,	233354,	237279,	241240,	245236,
	249269,	253338,	257442,	261583,	265760,	269974,	274223,	278509,
	282832,	287191,	291586,	296018,	300487,	304992,	309534,	314113,
	318729,	323381,	328071,	332797,	337561,	342362,	347199,	352074,
	356987,	361936,	366923,	371947,	377009,	382108,	387245,	392419,
	397631,	402881,	408168,	413493,	418856,	424257,	429695,	435172,
	440686,	446239,	451830,	457459,	463126,	468831,	474575,	480356,
	486177,	492035,	497932,	503868,	509842,	515854,	521906,	527996,
	534124,	540292,	546498,	552743,	559027,	565349,	571711,	578112,
	584552,	591030,	597548,	604106,	610702,	617337,	624012,	630726,
	637480,	644273,	651105,	657977,	664888,	671839,	678830,	685860,
	692930,	700039,	707189,	714378,	721607,	728876,	736184,	743533,
	750922,	758350,	765819,	773328,	780877,	788466,	796095,	803765,
	811475,	819225,	827015,	834846,	842717,	850629,	858582,	866574,
	874608,	882682,	890796,	898952,	907148,	915384,	923662,	931980,
	940339,	948740,	957181,	965662,	974185,	982749,	991354,	1000000
};

const u32 gamma_control_set_222[] = {
	0,	5,	22,	53,	99,	162,	243,	342,
	460,	597,	755,	932,	1131,	1351,	1592,	1856,
	2142,	2450,	2781,	3136,	3514,	3916,	4342,	4792,
	5267,	5767,	6292,	6841,	7417,	8017,	8644,	9297,
	9976,	10681,	11413,	12171,	12957,	13769,	14609,	15476,
	16371,	17293,	18243,	19222,	20228,	21263,	22326,	23418,
	24538,	25688,	26866,	28073,	29310,	30576,	31871,	33197,
	34551,	35936,	37351,	38795,	40270,	41775,	43311,	44877,
	46473,	48101,	49759,	51448,	53169,	54920,	56703,	58516,
	60362,	62239,	64147,	66088,	68060,	70064,	72100,	74168,
	76268,	78401,	80566,	82763,	84993,	87256,	89551,	91879,
	94240,	96634,	99061,	101521,	104014,	106541,	109100,	111694,
	114321,	116981,	119675,	122403,	125165,	127961,	130790,	133654,
	136551,	139483,	142450,	145450,	148485,	151555,	154659,	157797,
	160970,	164178,	167421,	170699,	174012,	177360,	180742,	184160,
	187614,	191102,	194626,	198185,	201780,	205410,	209076,	212778,
	216515,	220288,	224097,	227942,	231823,	235740,	239692,	243682,
	247707,	251768,	255866,	260001,	264171,	268379,	272623,	276903,
	281220,	285574,	289965,	294392,	298856,	303358,	307896,	312471,
	317084,	321734,	326421,	331145,	335906,	340705,	345541,	350415,
	355327,	360276,	365262,	370286,	375349,	380448,	385586,	390762,
	395975,	401227,	406516,	411844,	417210,	422614,	428056,	433537,
	439055,	444613,	450208,	455843,	461515,	467227,	472977,	478765,
	484593,	490459,	496364,	502307,	508290,	514312,	520372,	526472,
	532611,	538789,	545006,	551262,	557558,	563892,	570267,	576680,
	583133,	589626,	596158,	602729,	609341,	615992,	622682,	629412,
	636183,	642992,	649842,	656732,	663662,	670631,	677641,	684691,
	691781,	698911,	706081,	713291,	720542,	727833,	735165,	742537,
	749949,	757402,	764895,	772429,	780003,	787618,	795274,	802971,
	810708,	818486,	826305,	834165,	842065,	850007,	857989,	866013,
	874078,	882183,	890330,	898518,	906748,	915018,	923330,	931683,
	940078,	948514,	956991,	965510,	974070,	982672,	991315,	1000000
};

const u32 gamma_control_set_223[] = {
	0,	5,	21,	50,	95,	156,	234,	330,
	444,	578,	731,	904,	1097,	1311,	1547,	1804,
	2083,	2385,	2709,	3056,	3426,	3820,	4237,	4679,
	5144,	5635,	6150,	6689,	7255,	7845,	8461,	9103,
	9771,	10465,	11185,	11932,	12705,	13506,	14333,	15188,
	16070,	16980,	17917,	18883,	19876,	20897,	21947,	23025,
	24132,	25267,	26432,	27625,	28848,	30099,	31381,	32691,
	34031,	35402,	36802,	38231,	39692,	41182,	42703,	44254,
	45835,	47448,	49091,	50765,	52470,	54207,	55974,	57773,
	59603,	61465,	63359,	65284,	67241,	69230,	71251,	73304,
	75389,	77507,	79657,	81839,	84054,	86302,	88583,	90896,
	93243,	95622,	98034,	100480,	102959,	105471,	108017,	110596,
	113209,	115856,	118536,	121251,	123999,	126781,	129597,	132448,
	135332,	138251,	141205,	144193,	147215,	150272,	153364,	156490,
	159651,	162848,	166079,	169345,	172647,	175983,	179355,	182762,
	186205,	189683,	193196,	196746,	200330,	203951,	207607,	211300,
	215028,	218792,	222592,	226429,	230301,	234210,	238155,	242137,
	246155,	250209,	254300,	258428,	262592,	266793,	271031,	275306,
	279618,	283966,	288352,	292775,	297235,	301732,	306267,	310838,
	315448,	320094,	324779,	329500,	334260,	339057,	343891,	348764,
	353674,	358623,	363609,	368633,	373695,	378796,	383934,	389111,
	394326,	399580,	404871,	410202,	415570,	420977,	426423,	431908,
	437431,	442992,	448593,	454232,	459911,	465628,	471384,	477180,
	483014,	488887,	494800,	500752,	506743,	512774,	518843,	524953,
	531102,	537290,	543518,	549785,	556092,	562439,	568826,	575252,
	581718,	588224,	594770,	601356,	607982,	614649,	621355,	628101,
	634888,	641715,	648582,	655489,	662437,	669425,	676454,	683524,
	690633,	697784,	704975,	712207,	719479,	726793,	734147,	741542,
	748977,	756454,	763972,	771531,	779131,	786772,	794454,	802177,
	809942,	817748,	825595,	833483,	841413,	849385,	857398,	865452,
	873548,	881685,	889865,	898085,	906348,	914652,	922998,	931386,
	939816,	948288,	956801,	965357,	973955,	982595,	991276,	1000000
};

const u32 gamma_control_set_224[] = {
	0,	5,	20,	48,	91,	150,	226,	318,
	429,	559,	707,	876,	1064,	1273,	1503,	1754,
	2026,	2321,	2638,	2977,	3340,	3725,	4135,	4567,
	5024,	5505,	6011,	6541,	7096,	7676,	8282,	8913,
	9570,	10253,	10962,	11697,	12459,	13248,	14063,	14906,
	15775,	16672,	17597,	18549,	19530,	20538,	21574,	22639,
	23732,	24854,	26005,	27184,	28393,	29630,	30897,	32194,
	33520,	34875,	36261,	37676,	39121,	40597,	42103,	43639,
	45206,	46804,	48432,	50091,	51781,	53503,	55255,	57039,
	58854,	60701,	62580,	64490,	66432,	68406,	70412,	72450,
	74520,	76623,	78758,	80926,	83126,	85359,	87625,	89924,
	92256,	94621,	97019,	99450,	101915,	104413,	106944,	109510,
	112109,	114742,	117408,	120109,	122843,	125612,	128415,	131252,
	134124,	137030,	139971,	142946,	145956,	149000,	152080,	155194,
	158343,	161528,	164747,	168002,	171292,	174617,	177978,	181375,
	184806,	188274,	191777,	195316,	198891,	202502,	206149,	209832,
	213551,	217306,	221098,	224925,	228790,	232690,	236628,	240602,
	244612,	248659,	252744,	256864,	261022,	265217,	269449,	273718,
	278024,	282368,	286748,	291167,	295622,	300115,	304646,	309214,
	313820,	318463,	323145,	327864,	332621,	337416,	342249,	347120,
	352030,	356977,	361963,	366987,	372050,	377150,	382290,	387468,
	392684,	397939,	403233,	408566,	413937,	419347,	424796,	430285,
	435812,	441378,	446983,	452628,	458312,	464035,	469797,	475599,
	481440,	487321,	493241,	499201,	505201,	511240,	517319,	523438,
	529597,	535795,	542034,	548312,	554631,	560989,	567388,	573827,
	580307,	586826,	593386,	599987,	606627,	613309,	620030,	626793,
	633596,	640439,	647324,	654249,	661215,	668222,	675270,	682358,
	689488,	696659,	703871,	711124,	718418,	725753,	733130,	740548,
	748007,	755508,	763050,	770634,	778259,	785926,	793635,	801385,
	809177,	817010,	824886,	832803,	840762,	848763,	856806,	864891,
	873018,	881188,	889399,	897653,	905948,	914286,	922667,	931089,
	939555,	948062,	956612,	965205,	973840,	982517,	991238,	1000000
};

const u32 gamma_control_set_225[] = {
	0,	4,	19,	46,	88,	144,	217,	307,
	415,	540,	685,	849,	1032,	1235,	1460,	1705,
	1971,	2259,	2569,	2901,	3256,	3634,	4034,	4459,
	4907,	5379,	5875,	6396,	6941,	7511,	8107,	8727,
	9373,	10045,	10743,	11467,	12218,	12994,	13798,	14628,
	15486,	16370,	17283,	18222,	19190,	20185,	21208,	22259,
	23339,	24448,	25584,	26750,	27945,	29168,	30421,	31704,
	33015,	34357,	35728,	37129,	38559,	40020,	41512,	43033,
	44586,	46168,	47782,	49426,	51102,	52808,	54546,	56314,
	58115,	59947,	61810,	63705,	65632,	67591,	69583,	71606,
	73661,	75749,	77870,	80023,	82208,	84427,	86678,	88962,
	91279,	93630,	96014,	98431,	100881,	103365,	105883,	108434,
	111019,	113638,	116291,	118978,	121699,	124454,	127244,	130068,
	132926,	135819,	138747,	141710,	144707,	147739,	150806,	153909,
	157046,	160218,	163426,	166669,	169948,	173262,	176612,	179997,
	183419,	186876,	190369,	193898,	197462,	201063,	204701,	208374,
	212084,	215830,	219613,	223432,	227288,	231181,	235110,	239076,
	243079,	247119,	251196,	255311,	259462,	263650,	267876,	272139,
	276440,	280778,	285154,	289567,	294018,	298507,	303034,	307598,
	312200,	316841,	321519,	326236,	330991,	335784,	340615,	345485,
	350393,	355339,	360325,	365348,	370411,	375512,	380652,	385831,
	391049,	396306,	401601,	406936,	412310,	417723,	423176,	428668,
	434199,	439769,	445379,	451029,	456718,	462447,	468215,	474024,
	479872,	485760,	491687,	497655,	503663,	509711,	515799,	521927,
	528096,	534305,	540554,	546843,	553173,	559544,	565955,	572406,
	578899,	585432,	592005,	598620,	605275,	611971,	618709,	625487,
	632306,	639167,	646068,	653011,	659995,	667020,	674087,	681195,
	688345,	695536,	702768,	710042,	717358,	724715,	732115,	739556,
	747038,	754563,	762130,	769738,	777389,	785081,	792816,	800593,
	808412,	816273,	824177,	832123,	840111,	848142,	856215,	864331,
	872489,	880690,	888934,	897220,	905549,	913921,	922335,	930793,
	939293,	947836,	956423,	965052,	973724,	982440,	991199,	1000000
};

const u32 *GAMMA_CONTROL_TABLE[G_MAX] = {
	gamma_control_set_21,
	gamma_control_set_213,
	gamma_control_set_215,
	gamma_control_set_218,
	gamma_control_set_22,
	gamma_control_set_221,
	gamma_control_set_222,
	gamma_control_set_223,
	gamma_control_set_224,
	gamma_control_set_225
};

const struct str_flookup_table flookup_table[302] = {
	{  0,   0},  {  1,  20},
	{ 20,   7},  { 27,   5},
	{ 32,   4},  { 36,   4},
	{ 40,   4},  { 44,   3},
	{ 47,   3},  { 50,   2},
	{ 52,   3},  { 55,   2},
	{ 57,   3},  { 60,   2},
	{ 62,   2},  { 64,   2},
	{ 66,   2},  { 68,   2},
	{ 70,   1},  { 71,   2},
	{ 73,   2},  { 75,   2},
	{ 77,   1},  { 78,   2},
	{ 80,   1},  { 81,   2},
	{ 83,   1},  { 84,   2},
	{ 86,   1},  { 87,   2},
	{ 89,   1},  { 90,   1},
	{ 91,   2},  { 93,   1},
	{ 94,   1},  { 95,   2},
	{ 97,   1},  { 98,   1},
	{ 99,   1},  {100,   1},
	{101,   2},  {103,   1},
	{104,   1},  {105,   1},
	{106,   1},  {107,   1},
	{108,   1},  {109,   1},
	{110,   1},  {111,   1},
	{112,   1},  {113,   1},
	{114,   1},  {115,   1},
	{116,   1},  {117,   1},
	{118,   1},  {119,   1},
	{120,   1},  {121,   1},
	{122,   1},  {123,   1},
	{124,   1},  {125,   1},
	{126,   1},  {127,   1},
	{128,   1},  {129,   1},
	{  0,   0},  {130,   1},
	{131,   1},  {132,   1},
	{133,   1},  {134,   1},
	{  0,   0},  {135,   1},
	{136,   1},  {137,   1},
	{138,   1},  {139,   1},
	{  0,   0},  {140,   1},
	{141,   1},  {142,   1},
	{  0,   0},  {143,   1},
	{144,   1},  {145,   1},
	{146,   1},  {  0,   0},
	{147,   1},  {148,   1},
	{149,   1},  {  0,   0},
	{150,   1},  {151,   1},
	{  0,   0},  {152,   1},
	{153,   1},  {154,   1},
	{  0,   0},  {155,   1},
	{156,   1},  {  0,   0},
	{157,   1},  {158,   1},
	{  0,   0},  {159,   1},
	{160,   1},  {  0,   0},
	{161,   1},  {162,   1},
	{  0,   0},  {163,   1},
	{164,   1},  {  0,   0},
	{165,   1},  {166,   1},
	{  0,   0},  {167,   1},
	{168,   1},  {  0,   0},
	{169,   1},  {170,   1},
	{  0,   0},  {171,   1},
	{  0,   0},  {172,   1},
	{173,   1},  {  0,   0},
	{174,   1},  {  0,   0},
	{175,   1},  {176,   1},
	{  0,   0},  {177,   1},
	{  0,   0},  {178,   1},
	{179,   1},  {  0,   0},
	{180,   1},  {  0,   0},
	{181,   1},  {182,   1},
	{  0,   0},  {183,   1},
	{  0,   0},  {184,   1},
	{  0,   0},  {185,   1},
	{186,   1},  {  0,   0},
	{187,   1},  {  0,   0},
	{188,   1},  {  0,   0},
	{189,   1},  {  0,   0},
	{190,   1},  {191,   1},
	{  0,   0},  {192,   1},
	{  0,   0},  {193,   1},
	{  0,   0},  {194,   1},
	{  0,   0},  {195,   1},
	{  0,   0},  {196,   1},
	{  0,   0},  {197,   1},
	{198,   1},  {  0,   0},
	{199,   1},  {  0,   0},
	{200,   1},  {  0,   0},
	{201,   1},  {  0,   0},
	{202,   1},  {  0,   0},
	{203,   1},  {  0,   0},
	{204,   1},  {  0,   0},
	{205,   1},  {  0,   0},
	{206,   1},  {  0,   0},
	{207,   1},  {  0,   0},
	{208,   1},  {  0,   0},
	{209,   1},  {  0,   0},
	{210,   1},  {  0,   0},
	{211,   1},  {  0,   0},
	{212,   1},  {  0,   0},
	{213,   1},  {  0,   0},
	{  0,   0},  {214,   1},
	{  0,   0},  {215,   1},
	{  0,   0},  {216,   1},
	{  0,   0},  {217,   1},
	{  0,   0},  {218,   1},
	{  0,   0},  {219,   1},
	{  0,   0},  {220,   1},
	{  0,   0},  {221,   1},
	{  0,   0},  {  0,   0},
	{222,   1},  {  0,   0},
	{223,   1},  {  0,   0},
	{224,   1},  {  0,   0},
	{225,   1},  {  0,   0},
	{  0,   0},  {226,   1},
	{  0,   0},  {227,   1},
	{  0,   0},  {228,   1},
	{  0,   0},  {229,   1},
	{  0,   0},  {  0,   0},
	{230,   1},  {  0,   0},
	{231,   1},  {  0,   0},
	{232,   1},  {  0,   0},
	{233,   1},  {  0,   0},
	{  0,   0},  {234,   1},
	{  0,   0},  {235,   1},
	{  0,   0},  {  0,   0},
	{236,   1},  {  0,   0},
	{237,   1},  {  0,   0},
	{238,   1},  {  0,   0},
	{  0,   0},  {239,   1},
	{  0,   0},  {240,   1},
	{  0,   0},  {241,   1},
	{  0,   0},  {  0,   0},
	{242,   1},  {  0,   0},
	{243,   1},  {  0,   0},
	{  0,   0},  {244,   1},
	{  0,   0},  {245,   1},
	{  0,   0},  {  0,   0},
	{246,   1},  {  0,   0},
	{247,   1},  {  0,   0},
	{  0,   0},  {248,   1},
	{  0,   0},  {249,   1},
	{  0,   0},  {  0,   0},
	{250,   1},  {  0,   0},
	{251,   1},  {  0,   0},
	{  0,   0},  {252,   1},
	{  0,   0},  {253,   1},
	{  0,   0},  {  0,   0},
	{254,   1},  {  0,   0},
	{  0,   0},  {255,   1},
};

#endif
