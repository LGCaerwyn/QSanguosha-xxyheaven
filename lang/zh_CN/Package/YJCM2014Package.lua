-- translation for YJCM2014 Package

return {
	["YJCM2014"] = "一将成名2014",

	["#caifuren"] = "襄江的蒲苇",
	["caifuren"] = "蔡夫人",
	["illustrator:caifuren"] = "Dream彼端",
	["designer:caifuren"] = "B.LEE",
	["qieting"] = "窃听",
	[":qieting"] = "其他角色的回合结束后，若其未于此回合内使用过以另一名角色为目标的牌，则你可以选择一项：将其装备区里的一张牌置入你的装备区；或摸一张牌。",
	["qieting:0"] = "移动武器牌",
	["qieting:1"] = "移动防具牌",
	["qieting:2"] = "移动+1坐骑",
	["qieting:3"] = "移动-1坐骑",
	["qieting:4"] = "移动宝物牌",
	["qieting:draw"] = "摸一张牌",
	["xianzhou"] = "献州",
	[":xianzhou"] = "限定技，出牌阶段，你可以将装备区里的所有牌交给一名其他角色，然后该角色选择一项：令你回复X点体力；或对其攻击范围内的至多X名角色各造成1点伤害(X为你以此法交给该角色的牌的数量)。",
	["@xianzhou-damage"] = "请对攻击范围内的 %arg 名角色各造成 1 点伤害",
	["~xianzhou"] = "选择若干名角色→点击确定",
	["$XianzhouAnimate"] = "image=image/animate/xianzhou.png",

	["#caozhen"] = "荷国天督",
	["caozhen"] = "曹真",
	["illustrator:caozhen"] = "Thinking",
	["designer:caozhen"] = "世外高v狼",
	["sidi"] = "司敌",
	[":sidi"] = "当你使用或其他角色于你的回合内使用【闪】时，你可以将牌堆顶的一张牌置于武将牌上，称为“钤”；其他角色的出牌阶段开始时，你可以将一张“钤”牌置入弃牌堆。若如此做，该角色于此阶段内使用【杀】的次数上限-1。",
	["@sidi-remove"] = "你可以将一张“司敌牌”置入弃牌堆，令当前回合角色本回合使用【杀】的次数上限-1",
	["~sidi"] = "选择一张“司敌牌”→点击确定",
	
	["#chenqun"] = "万世臣表",
	["chenqun"] = "陈群",
	["illustrator:chenqun"] = "DH",
	["designer:chenqun"] = "To Joanna",
	["dingpin"] = "定品",
	[":dingpin"] = "出牌阶段，你可以弃置一张手牌（若你于此回合内使用或被弃置过牌，则此牌的类别须与这些牌均不同）并选择一名已受伤的角色，其进行判定，若结果为：黑色，该角色摸X张牌（X为其已损失的体力值），然后你于此回合内不能对其发动“定品”；红色，你翻面。",
	["faen"] = "法恩",
	[":faen"] = "当一名角色翻面或横置后，你可以令其摸一张牌。",

	["#guyong"] = "庙堂的玉磬",
	["guyong"] = "顾雍",
	["illustrator:guyong"] = "大佬荣",
	["designer:guyong"] = "睿笛终落",
	["shenxing"] = "慎行",
	[":shenxing"] = "出牌阶段，你可以弃置两张牌，然后摸一张牌。",
	["bingyi"] = "秉壹",
	["bingyiask"] = "秉壹",
	[":bingyi"] = "结束阶段开始时，你可以展示所有手牌，若颜色均相同，你选择至多X名角色（X为你的手牌数），然后这些角色各摸一张牌。",
	["@bingyi-card"] = "你发动了“秉壹”，请选择至多 %arg 名角色各摸一张牌",
	["~bingyiask"] = "选择摸牌的角色→点击确定",

	["#hanhaoshihuan"] = "中军之主",
	["hanhaoshihuan"] = "韩浩＆史涣",
	["&hanhaoshihuan"] = "韩浩史涣",
	["illustrator:hanhaoshihuan"] = "L",
	["designer:hanhaoshihuan"] = "浪人兵法家",
	["shenduan"] = "慎断",
	[":shenduan"] = "当你的一张黑色基本牌因弃置而置入弃牌堆后，你可以将此牌当【兵粮寸断】使用（无距离限制）。",
	["#shenduan"] = "慎断",
	["@shenduan-use"] = "你可以发动“慎断”将其中一张牌当【兵粮寸断】使用（无距离限制）",
	["~shenduan"] = "选择一张黑色基本牌→选择【兵粮寸断】的目标角色→点击确定",
	["yonglve"] = "勇略",
	[":yonglve"] = "其他角色的判定阶段开始时，若其在你的攻击范围内，则你可以弃置其判定区里的一张牌。若如此做，视为你对其使用一张【杀】，然后当此【杀】结算完毕后，若此【杀】未造成过伤害，则你摸一张牌。",
	["#yonglve"] = "勇略",
	["@yonglve-use"] = "你可以对 %src 发动“勇略”",
	["~yonglve"] = "选择该角色判定区内的一张牌→点击确定",

	["#jvshou"] = "监军谋国",
	["jvshou"] = "沮授",
	["illustrator:jvshou"] = "酱油之神",
	["designer:jvshou"] = "精精神神",
	["jianying"] = "渐营",
	[":jianying"] = "当你于出牌阶段内使用牌时，若此牌与你于此阶段内使用的上一张牌点数或花色相同，则你可以摸一张牌。",
	["shibei"] = "矢北",
	[":shibei"] = "锁定技，当你受到伤害后，若此伤害：是你本回合第一次受到伤害，则你回复1点体力；不是你本回合第一次受到伤害，则你失去1点体力。",

	["#sunluban"] = "为虎作伥",
	["sunluban"] = "孙鲁班",
	["illustrator:sunluban"] = "FOOLTOWN",
	["designer:sunluban"] = "CatCat44",
	["zenhui"] = "谮毁",
	[":zenhui"] = "出牌阶段限一次，当你于出牌阶段内使用【杀】或黑色普通锦囊牌指定目标时，若此牌的目标角色数为1，你可令可以成为此牌目标的另一名其他角色选择一项：交给你一张牌，然后代替你成为此牌的使用者；或也成为此牌的目标。",
	["zenhui-invoke"] = "你可以发动“谮毁”<br/> <b>操作提示</b>: 选择除 %src 外的一名角色→点击确定<br/>",
	["@zenhui-collateral"] = "请选择【借刀杀人】 %src 使用【杀】的目标",
	["@zenhui-give"] = "请交给 %src 一张牌成为此牌的使用者，否则你成为此牌的目标",
	["jiaojin"] = "骄矜",
	[":jiaojin"] = "当你受到男性角色造成的伤害时，你可以弃置一张装备牌。若如此做，此伤害-1。",
	["@jiaojin"] = "你可以弃置一张装备牌发动“骄矜”令此伤害-1",
	["#Jiaojin"] = "%from 发动了“<font color=\"yellow\"><b>骄矜</b></font>”，伤害从 %arg 点减少至 %arg2 点",

	["#wuyi"] = "建兴鞍辔",
	["wuyi"] = "吴懿",
	["illustrator:wuyi"] = "蚂蚁君",
	["designer:wuyi"] = "沸治.克里夫",
	["benxi"] = "奔袭",
	[":benxi"] = "锁定技，当你于回合内使用牌时，直到回合结束，你计算与其他角色的距离-1；你的回合内，若你与所有其他角色的距离均为1，则你无视其他角色的防具且你使用【杀】的目标数上限+1。",
	["#benxi-dist"] = "奔袭",

	["#zhangsong"] = "怀璧待凤仪",
	["zhangsong"] = "张松",
	["illustrator:zhangsong"] = "尼乐小丑",
	["designer:zhangsong"] = "冷王无双",
	["qiangzhi"] = "强识",
	[":qiangzhi"] = "出牌阶段开始时，你可以展示一名其他角色的一张手牌。若如此做，直到此阶段结束，当你使用与展示的牌类别相同的牌时，你可以摸一张牌。",
	["qiangzhi-invoke"] = "你可以发动“强识”<br/> <b>操作提示</b>: 选择一名有手牌的其他角色→点击确定<br/>",
	["xiantu"] = "献图",
	[":xiantu"] = "其他角色的出牌阶段开始时，你可以摸两张牌，然后将两张牌交给该角色。此阶段结束时，若其未于此阶段内杀死过角色，则你失去1点体力。",
	["@xiantu-give"] = "请交给 %dest %arg 张牌",
	["#Xiantu"] = "%from 未于本阶段杀死过角色，%to 的“%arg”被触发",

	["#zhoucang"] = "披肝沥胆",
	["zhoucang"] = "周仓",
	["illustrator:zhoucang"] = "Sky",
	["designer:zhoucang"] = "WOLVES29",
	["zhongyong"] = "忠勇",
	[":zhongyong"] = "当你于出牌阶段内使用的【杀】被目标角色使用的【闪】抵消后，你可以将此【闪】交给除该角色外的一名角色。若获得此【闪】的角色不是你，则你可以对相同的目标使用一张【杀】。",
	["zhongyong-invoke"] = "你可以发动“忠勇”<br/> <b>操作提示</b>: 选择除 %src 外的一名角色→点击确定<br/>",
	["zhongyong-slash"] = "你可以发动“忠勇”再对 %src 使用一张【杀】",

	["#zhuhuan"] = "中洲拒天人",
	["zhuhuan"] = "朱桓",
	["illustrator:zhuhuan"] = "XXX",
	["designer:zhuhuan"] = "半缘修道",
	["youdi"] = "诱敌",
	[":youdi"] = "结束阶段开始时，你可以令一名其他角色弃置你的一张牌，若其以此法弃置的牌不为【杀】，你获得其一张牌。",
	["youdi-invoke"] = "你可以发动“诱敌”<br> <b>操作提示</b>：选择一名其他角色→点击确定<br/>",
	["youdi_obtain"] = "诱敌获得牌",
}