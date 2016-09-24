-- translation for YJCM2015 Package

return {
	["YJCM2015"] = "一将成名2015",

	["#caorui"] = "天姿的明君",
	["caorui"] = "曹叡",
	["designer:caorui"] = "Ptolemy_M7",
	["illustrator:caorui"] = "Thinking",
	["huituo"] = "恢拓",
	[":huituo"] = "每当你受到伤害后，你可以令一名角色进行一次判定，若结果为红色，该角色回复1点体力；若结果为黑色，该角色摸X张牌（X为此次伤害的伤害数）。",
    ["@huituo-select"] = "你可以发动“恢拓”。\n操作提示：选择一名角色→点击确定" ,
	["mingjian"] = "明鉴",
	[":mingjian"] = "出牌阶段限一次，你可以将所有手牌交给一名其他角色。若如此做，该角色于其下回合的手牌上限+1，且出牌阶段内使用【杀】的次数上限+1。",
	["@mingjian"] = "明鉴",
	["xingshuai"] = "兴衰",
	[":xingshuai"] = "主公技，限定技，当你进入濒死状态时，你可以令其他魏势力角色选择是否令你回复1点体力，若其选择是，当此濒死结算结束后，其受到1点无来源的伤害。",
    ["_xingshuai:xing"] = "%src 发动“兴衰”，是否令其回复1点体力" ,

	["#caoxiu"] = "千里骐骥",
	["caoxiu"] = "曹休",
	["designer:caoxiu"] = "蹩脚狐小三",
	["illustrator:caoxiu"] = "eshao111",
	["qianju"] = "千驹",
	[":qianju"] = "锁定技，你计算与其他角色的距离-X（X为你已损失的体力值）。",
	["qingxi"] = "倾袭",
	[":qingxi"] = "当你使用【杀】对目标角色造成伤害时，若你的装备区里有武器牌，你可令其选择一项：弃置X张手牌（X为此武器牌的攻击范围），然后其弃置你的此武器牌；或令此伤害+1。",
	["@qingxi-discard"] = "请弃置 %arg 张手牌，否则伤害+1",

	["#gongsunyuan"] = "狡徒悬海",
	["gongsunyuan"] = "公孙渊",
	["designer:gongsunyuan"] = "死水微澜",
	["illustrator:gongsunyuan"] = "尼乐小丑",
	["huaiyi"] = "怀异",
	[":huaiyi"] = "出牌阶段限一次，你可以展示所有手牌，若不为同一颜色，则你弃置其中一种颜色的所有手牌，然后选择至多X名角色，获得这些角色的各一张牌（X为你以此法弃置的手牌数），若你以此法获得至少两张牌，则你失去1点体力。",
	["@huaiyi-choose"] = "怀异：请选择一种颜色",
	["@huaiyi"] = "你可以选择至多 %arg 名角色，获得他们的各一张牌",

	["#guotupangji"] = "凶蛇两端",
	["guotupangji"] = "郭图＆逢纪",
	["&guotupangji"] = "郭图逢纪",
	["designer:guotupangji"] = "辰木",
	["illustrator:guotupangji"] = "Aimer＆Vwolf",
	["jigong"] = "急攻",
	[":jigong"] = "出牌阶段开始时，你可以摸两张牌，若如此做，此回合你的手牌上限为X（X为你此阶段造成的伤害点数）。",
	["shifei"] = "饰非",
	[":shifei"] = "当你需要使用或打出【闪】时，你可以令当前回合角色摸一张牌，然后若其手牌数不为全场最多，则你弃置全场手牌数最多（或之一）角色的一张牌，视为你使用或打出了一张【闪】。",
    ["@shifei-dis"] = "请选择一名手牌数最多的角色",

	["#liuchen"] = "血荐轩辕",
	["liuchen"] = "刘谌",
	["designer:liuchen"] = "列缺霹雳",
	["illustrator:liuchen"] = "凌天翼＆depp",
	["zhanjue"] = "战绝",
	[":zhanjue"] = "你可以将所有手牌当【决斗】使用，然后你和以此法受到伤害的角色各摸一张牌，若你在同一阶段内以此法摸过两张或更多的牌，则此技能失效直到回合结束。",
	["qinwang"] = "勤王",
	[":qinwang"] = "主公技，当你需要使用或打出【杀】时，你可以弃置一张牌，令其他蜀势力角色选择是否打出一张【杀】（视为由你使用或打出），然后若有角色因响应而打出过【杀】，其摸一张牌。",
    ["@qinwang-discard"] = "你可以弃置一张牌发动“勤王”" ,
    ["@qinwang-slash"] = "请打出一张【杀】响应 %src “勤王”",
    
	["#quancong"] = "慕势耀族",
	["quancong"] = "全琮",
	["designer:quancong"] = "凌风自舞",
	["illustrator:quancong"] = "小小鸡仔",
	["zhenshan"] = "振赡",
	[":zhenshan"] = "当你需要使用或打出基本牌时，你可与不是此牌的目标且手牌数少于你的一名角色交换手牌（每名角色的回合内限一次）。若如此做，视为你使用或打出了此牌。",
	["$ZhenshanExchange"] = "%from 发动武将技能“%arg”与 %to 交换手牌，视为使用/打出【%arg2】",
	["zhenshan_slash"] = "振赡出杀",
	["zhenshan_saveself"] = "振赡自救",

	["#sunxiu"] = "弥殇的景君",
	["sunxiu"] = "孙休",
	["designer:sunxiu"] = "顶尖对决＆剑",
	["illustrator:sunxiu"] = "XXX",
	["yanzhu"] = "宴诛",
	[":yanzhu"] = "出牌阶段限一次，你可以令一名有牌的其他角色选择一项：令你获得其装备区里的所有牌，然后你失去此技能；或弃置一张牌。",
	["@yanzhu-discard"] = "请弃置一张牌，否则 %src 将获得你的所有装备",
    ["xingxue"] = "兴学",
	[":xingxue"] = "结束阶段开始时，你可以选择至多X名角色（若你：拥有技能“宴诛”，则X为你的体力值；没有技能“宴诛”，则X为你的体力上限），令这些角色依次摸一张牌并将一张牌置于牌堆顶。",
	["@xingxue-put"] = "请将一张牌置于牌堆顶" ,
    ["@xingxue"] = "你可以发动“兴学”",
    ["zhaofu"] = "诏缚",
	[":zhaofu"] = "主公技，锁定技，你距离为1的角色视为在除其外的其他吴势力角色的攻击范围内。",

	["#xiahoushi"] = "采缘撷睦",
	["xiahoushi"] = "夏侯氏",
	["designer:xiahoushi"] = "淬毒",
	["illustrator:xiahoushi"] = "2B铅笔",
	["qiaoshi"] = "樵拾",
	[":qiaoshi"] = "其他角色的结束阶段开始时，若其手牌数等于你，你可以令你与其各摸一张牌。",
	["yanyu"] = "燕语",
	[":yanyu"] = "出牌阶段，你可以重铸【杀】；出牌阶段结束时，若你于此阶段内以此法重铸过两张或更多的【杀】，则你可以令一名男性角色摸两张牌。",
	["@yanyu-give"] = "你可以令一名男性角色摸两张牌。",

	["#zhangni"] = "通壮逾古",
	["zhangni"] = "张嶷",
	["designer:zhangni"] = "XYZ",
	["illustrator:zhangni"] = "livsinno",
	["furong"] = "怃戎",
	[":furong"] = "出牌阶段限一次，若你有手牌，则你可以令你与一名有手牌的其他角色同时展示一张手牌：然后若你以此法展示的牌为【杀】且其以此法展示的牌不为【闪】，你弃置此【杀】，然后对其造成1点伤害。若你以此法展示的牌不为【杀】且其以此法展示的牌为【闪】，你弃置你展示的牌，然后获得其一张牌。",
	["@furong-source"] = "你发动了“怃戎”，请展示一张手牌",
	["@furong-target"] = "%src 对你发动“怃戎”，请展示一张手牌",
    ["shizhi"] = "矢志",
	[":shizhi"] = "锁定技，若你的体力值为1，你的【闪】视为【杀】",

	["#zhongyao"] = "正楷萧曹",
	["zhongyao"] = "钟繇",
	["designer:zhongyao"] = "怀默",
	["illustrator:zhongyao"] = "eshao111",
	["huomo"] = "活墨",
	[":huomo"] = "当你需要使用与你于当前回合内使用过的牌牌名不同的基本牌时，你可以将一张不为基本牌的黑色牌置于牌堆顶。若如此做，你视为使用此基本牌。",
	["huomo_slash"] = "活墨出杀",
	["huomo_saveself"] = "活墨自救",
	["zuoding"] = "佐定",
	[":zuoding"] = "当其他角色于其出牌阶段内使用黑桃牌指定目标后，若所有角色于此阶段内均未受到过伤害，则你可以令其中的一个目标摸一张牌。",
	["@zuoding"] = "请选择此牌的一名目标角色，令其摸一张牌",

	["#zhuzhi"] = "王事靡盬",
	["zhuzhi"] = "朱治",
	["designer:zhuzhi"] = "May＆Roy",
	["illustrator:zhuzhi"] = "心中一凛",
	["anguo"] = "安国",
	[":anguo"] = "出牌阶段限一次，你可以令一名其他角色获得其装备区里你选择的一张牌，然后若其攻击范围内的角色数因此而变小，你摸一张牌。",

}