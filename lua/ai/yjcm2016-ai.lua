
--成长期矫诏，只能当杀使用，需要队友。















sgs.ai_skill_invoke.danxin = function(self)
	return true
end

sgs.ai_skill_choice.danxin = function(self, choices)
	return "modify"
end




local duliang_skill = {}
duliang_skill.name = "duliang"
table.insert(sgs.ai_skills, duliang_skill)
duliang_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("DuliangCard") then return end
	return sgs.Card_Parse("@DuliangCard=.")
end

sgs.ai_skill_use_func.DuliangCard = function(card, use, self)
	local duliangrestraint = function(to)
		if self:hasSkills("yongsi|shelie|biluan|shensu|shuangxiong|qiaobian|luoyi|zaiqi|nostuxi|jijiu|beige", to) then
			return true
		end
		if self:hasSkills(sgs.notActive_cardneed_skill, to) then
			return true
		end
	end
	local findduliangtarget = function()
        self:sort(self.friends, "handcard")
		self.friends = sgs.reverse(self.friends)
		self:sort(self.enemies, "handcard")
		local players = sgs.QList2Table(self.room:getAllPlayers())
		self:sort(players, "handcard")
		table.removeTable(players, self.friends)
		table.removeTable(players, self.enemies)
		--感觉不用这么复杂，回头再看了。
		local _friends, _enemies = {}, {}
		for _, p in ipairs(self.friends) do
			if not p:isKongcheng() and not duliangrestraint(p) and p:objectName() ~= self.player:objectName() then
				table.insert(_friends, p)
			end
		end
		for _, p in ipairs(self.enemies) do
			if (self:needKongcheng(p) and p:getHandcardNum() == 1) or p:hasSkill("tuxi") or p:objectName() == self.player:objectName() then continue end
			if not p:isKongcheng() and not self:doNotDiscard(p, "he", true, 1) then
				table.insert(_enemies, p)
			end
		end
		for _, p in ipairs(_friends) do
			if (self:needKongcheng(p) and p:getHandcardNum() == 1) or self:doNotDiscard(p, "he", true, 1) or p:hasSkill("tuxi") then
				return p 
			end
		end
		for _, p in ipairs(_enemies) do
			if p:getHandcardNum() < 3 or duliangrestraint(p) then
				return p
			end
		end
		for _, p in ipairs(_friends) do
			if p:getHandcardNum() > 3 then
				return p 
			end
		end
		for _, p in ipairs(_enemies) do
			if p:getHandcardNum() < 3 then
				return p
			end
		end
		for _, p in ipairs(players) do
			if not p:isKongcheng() and not self:doNotDiscard(p, "he", true, 1) and p:objectName() ~= self.player:objectName() then
				return p
			end
		end
		if #_enemies > 0 then
			return _enemies[1]
		end
		if #_friends > 0 then
			return _friends[1]
		end
		return nil
	end
    local target = findduliangtarget()
	if target then
		use.card = card
        if use.to then
            use.to:append(target)
        end
	end
end

sgs.ai_use_value.DuliangCard = sgs.ai_use_value.Snatch + 2
sgs.ai_use_priority.DuliangCard = sgs.ai_use_priority.Snatch + 1

sgs.ai_card_intention.DuliangCard = function(self, card, from, tos)
	local target = tos[1]
	if target:hasSkill("tuxi") or (self:needKongcheng(target) and target:getHandcardNum() == 1) or self:doNotDiscard(target, "he", true, 1) then
		sgs.updateIntentions(from, tos, -10)
	else
		sgs.updateIntentions(from, tos, 10)
	end
end

sgs.ai_skill_choice.duliang = function(self, choices)
	return "delay"
end

local kuangbi_skill = {}
kuangbi_skill.name = "kuangbi"
table.insert(sgs.ai_skills, kuangbi_skill)
kuangbi_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("KuangbiCard") then return end
	return sgs.Card_Parse("@KuangbiCard=.")
end

sgs.ai_skill_use_func.KuangbiCard = function(card, use, self)
	local findkuangbitarget = function ()
        self:sort(self.friends, "handcard")
		self.friends = sgs.reverse(self.friends)
		for _, p in ipairs(self.friends) do
			if p:isNude() then continue end
			if ((p:hasSkills(sgs.lose_equip_skill) and p:hasEquip()) or self:needKongcheng(p)) and p:objectName() ~= self.player:objectName() then
				return p
			end
		end
		local standby
		if #self.friends > 0 then
			standby = self.friends[1]
			if standby:objectName() == self.player:objectName() then
				standby = nil
				if #self.friends > 1 then
					standby = self.friends[2]
				end
			end
			if standby:getCardCount(true) > 3 then
				return standby
			end
		end
		self:sort(self.enemies, "handcard")
		for _, p in ipairs(self.enemies) do
			if ((p:hasSkills(sgs.lose_equip_skill) and p:hasEquip()) or self:needKongcheng(p) or p:isNude()) then continue end
			if p:getCardCount(true) < 3 and not self:doNotDiscard(p, "he", true, 1) then
				return p
			end
		end
		if standby and not self:isWeak(standby) and not standby:isNude() then
			return standby
		end
		local players = sgs.QList2Table(self.room:getAllPlayers())
		self:sort(players, "handcard")
		for _, p in ipairs(players) do
			if not self:isFriend(p) and not p:isNude() then
				return p
			end
		end
		return nil
	end
    local target = findkuangbitarget()
	if target then
		use.card = card
        if use.to then
            use.to:append(target)
        end
	end
end

sgs.ai_use_value.KuangbiCard = 7
sgs.ai_use_priority.KuangbiCard = 2

sgs.ai_skill_discard.kuangbi = function(self, discard_num, min_num, optional, include_equip)
	local sundeng = self.room:getCurrent()
	if not sundeng or not self:isFriend(sundeng) then
		return self:askForDiscard("dummy", min_num, min_num, optional, include_equip)
	end
	local to_discard = {}
	local max_n = discard_num
	local give_equips = {}
	if self.player:getArmor() and self:needToThrowArmor() then
		table.insert(give_equips, self.player:getArmor():getId())
		max_n = max_n-1
	end
	if self:needKongcheng() and max_n >= self.player:getHandcardNum() then
		table.insertTable(to_discard, self:askForDiscard("dummy", max_n, max_n, false, false))
	else
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(cards)
		local card_ids = {}
		for _,card in ipairs(cards) do
			table.insert(card_ids, card:getEffectiveId())
		end
		to_discard = self:askForDiscard("dummy", discard_num, discard_num, false, include_equip)
		local temp = table.copyFrom(to_discard)
		local keep_jink = 1
		local players = sgs.QList2Table(self.room:getAllPlayers())
		local to_seat = (self.player:getSeat() - sundeng:getSeat()) % #players
		local enemynum = 0
		for _, p in ipairs(players) do
			if self:isEnemy(p) and ((p:getSeat() - sundeng:getSeat()) % #players) < to_seat and p:inMyAttackRange(self.player) then
				enemynum = enemynum + 1
			end
		end
		if self:isWeak() then
			keep_jink = enemynum
		else
			if self:needToLoseHp() then
				keep_jink = 0
			else
				keep_jink = math.min(keep_jink, enemynum)
			end
		end
		
		for i = 1, #temp, 1 do
			local card = sgs.Sanguosha:getCard(temp[i])
			if self:isValuableCard(card) then
				table.removeOne(to_discard, temp[i])
				continue
			end
			if isCard("Jink", card, self.player) and keep_jink > 0 then
				table.removeOne(to_discard, temp[i])
				keep_jink = keep_jink-1 
				continue
			end
		end
	end
	if #to_discard == 0 then
		return self:askForDiscard("dummy", min_num, min_num, optional, include_equip)
	end
	return to_discard
end

local zhige_skill = {}
zhige_skill.name = "zhige"
table.insert(sgs.ai_skills, zhige_skill)
zhige_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ZhigeCard") or self.player:getHandcardNum() <= self.player:getHp() then return end
	return sgs.Card_Parse("@ZhigeCard=.")
end

sgs.ai_skill_use_func.ZhigeCard = function(card, use, self)
	local hasslashtozhige = function(target)
		if target:getHandcardNum() > 3 or target:hasSkill("aocai") then return true end
		local cards = sgs.QList2Table(target:getHandcards())
		local flag = string.format("%s_%s_%s","visible", self.player:objectName(), target:objectName())
		for _, cc in ipairs(cards) do
			if (cc:hasFlag("visible") or cc:hasFlag(flag)) and isCard("Slash", cc, target) then
				return true
			end
		end
		if target:hasSkill("wusheng") then
			if target:getHandcardNum() > 1 then return true end
			for _, card in sgs.qlist(target:getEquips()) do
				if card:isRed() then return true end
			end
		end
		if target:hasSkill("longdan") then
			if target:getHandcardNum() > 2 then return true end
		end
		if target:hasSkill("longhun") then
			if target:getHp() == 1 then
				if target:getHandcardNum() > 2 then return true end
				for _, card in sgs.qlist(target:getEquips()) do
					if card:getSuit() == sgs.Card_Diamond then return true end
				end
			end
		end
		if target:hasSkill("huomo") then
			for _, card in sgs.qlist(target:getEquips()) do
				if card:isBlack() then return true end
			end
		end
		return false
	end
	local zhigepige = function(urgent)
		for _, p in ipairs(self.friends) do
			if not p:inMyAttackRange(self.player) or not hasslashtozhige(p) then continue end
			for _, t in sgs.qlist(self.room:getAlivePlayers()) do
				local slash = sgs.Sanguosha:cloneCard("slash")
				if p:canSlash(t, nil, true) and self:isEnemy(t) and self:slashIsEffective(slash, t, p) then
					if urgent then
						if self:isWeak(t) or self:hasHeavySlashDamage(p, slash, to) then
							return p
						end
						continue
					end
					return p
				end
			end
		end
		return nil
	end
	local findzhigetarget = function()
		--武将定位：配合队友打二刀伤害（注意，目标不一定需要有装备）。配合凌统、香香
		--抢敌人装备，凌统，香香除外
		self:sort(self.friends, "handcard")
		self.friends = sgs.reverse(self.friends)
		local target = zhigepige(true)
		if target then return target end
		for _, p in ipairs(self.friends) do
			if (self:needToThrowArmor() or (p:hasEquip() and self:hasSkills(sgs.lose_equip_skill, p))) and p:inMyAttackRange(self.player) then
				return p
			end
		end
		self:sort(self.enemies, "handcard")
		for _, p in ipairs(self.enemies) do
			if p:inMyAttackRange(self.player) and p:hasEquip() and not self:hasSkills(sgs.lose_equip_skill, p) and not self:needToThrowArmor() then
				if hasslashtozhige(p) or p:getHandcardNum() > 2 then
					local hastarget = false
					for _, t in sgs.qlist(self.room:getAlivePlayers()) do
						if p:inMyAttackRange(t) and self:isFriend(t) then
							hastarget = true
						end
					end
					if hastarget then continue end
				end
				return p
			end
		end
		local target = zhigepige(false)
		if target then return target end
		return nil
	end
	local target = findzhigetarget()
	if target then
		use.card = card
        if use.to then
            use.to:append(target)
        end
	end
	return nil
end

sgs.ai_use_priority.ZhigeCard = 10

local jishe_skill = {}
jishe_skill.name = "jishe"
table.insert(sgs.ai_skills, jishe_skill)
jishe_skill.getTurnUseCard = function(self)
	if self.player:getMaxCards() > 0 then
		return sgs.Card_Parse("@JisheCard=.")
	end
end

sgs.ai_skill_use_func.JisheCard=function(card, use, self)
	use.card = card
end

sgs.ai_use_value.JisheCard = 4.4
sgs.ai_use_priority.JisheCard = 9.4


sgs.ai_skill_use["@@jishe"] = function(self, prompt, method)
	
	return "@JisheChainCard=.->"
end



sgs.ai_skill_playerchosen.qinqing = function(self, targets)
	local the_lord = self.room:getLord()
	--拆装备、过牌
	local dis_equip = self:findPlayerToDiscard("e", false, true, targets, true)
	if #dis_equip > 0 then
		for _, p in ipairs(dis_equip) do
			if p:getHandcardNum() >= the_lord:getHandcardNum() then
				return p
			end
		end
		return dis_equip[1]
	end

	self:sort(self.friends, "handcard")
	self.friends = sgs.reverse(self.friends)
	self:sort(self.enemies, "handcard")
	for _, p in ipairs(self.enemies) do
		if p:getHandcardNum() > the_lord:getHandcardNum() and not self:doNotDiscard(p, "he") and targets:contains(p) then
			return p
		end
	end
	for _, p in ipairs(self.friends) do
		if (p:getHandcardNum() > the_lord:getHandcardNum() or self:doNotDiscard(p, "he")) and targets:contains(p) then
			return p
		end
	end
	--找找明桃啥的
	for _, enemy in ipairs(self.enemies) do
		if not targets:contains(p) then continue end
		local cards = sgs.QList2Table(enemy:getHandcards())
		local flag = string.format("%s_%s_%s","visible", self.player:objectName(), enemy:objectName())
		if #cards <= 2 and not enemy:isKongcheng() and not (enemy:hasSkills("tuntian+zaoxian") and enemy:getPhase() == sgs.Player_NotActive) then
			for _, cc in ipairs(cards) do
				if (cc:hasFlag("visible") or cc:hasFlag(flag)) and (cc:isKindOf("Peach") or cc:isKindOf("Analeptic")) and self.player:canDiscard(enemy, cc:getId()) then
					return enemy
				end
			end
		end
	end
end

--

sgs.ai_skill_cardask["@huisheng-show"] = function(self, data)
	local damage = data:toDamage()
	local target = damage.from
	if not target or target:isDead() then return false end
	if self:needToLoseHp(self.player, target, (damage.card and damage.card:isKindOf("Slash"))) then return "" end
	local select_cards = {}
	if self:isFriend(target) then
		--队友，无脑让看全部牌
		for _, c in sgs.qlist(self.player:getCards("he")) do
			table.insert(select_cards, c:getId())
		end
		if #select_cards > 0 then
			return "$" .. table.concat(select_cards, "+")
		end
	else
		--敌人，给看伤害点数+1张牌，至多为目标的牌数+1
		local n = math.min(damage.damage, target:getCardCount(true)) + 1
		local min_n = damage.damage + 1
		if self.player:getArmor() and self:needToThrowArmor() then
			table.insert(select_cards, self.player:getArmor():getId())
		end
		local cards = self.player:getHandcards()
		cards = sgs.QList2Table(cards)
		self:sortByKeepValue(cards)
		for _, c in ipairs(cards) do
			if #select_cards >= n then break end
			if not self:isValuableCard(c) then
				table.insert(select_cards, c:getId())
			end
		end
		if #select_cards < min_n then
			if not self:hasSkills(sgs.lose_equip_skill, target) then
				--考虑给下装备
				local can_give_weapon, can_give_horse = true, true
				self:sort(self.friends, "defense")
				local friend = self.friends[1]
				if not target:inMyAttackRange(friend) then
					local a, b = 0, 0
					if self.player:getWeapon() then
						local weapon = self.player:getWeapon():getRealCard():toWeapon()
						a = weapon:getRange() - 1
					end
					if target:getWeapon() then
						local weapon = target:getWeapon():getRealCard():toWeapon()
						b = weapon:getRange() - 1
					end
					
					if target:distanceTo(friend, a - b) <= target:getAttackRange() then
						can_give_weapon = false
					end
					if self.player:getOffensiveHorse() and not target:getOffensiveHorse() then
						if target:distanceTo(friend, 1) <= target:getAttackRange() then
							can_give_horse = false
						end
					end
					
				end
				if can_give_weapon and self.player:getWeapon() then
					table.insert(select_cards, self.player:getWeapon():getId())
				end
				if #select_cards < min_n and can_give_horse and self.player:getOffensiveHorse() then
					table.insert(select_cards, self.player:getOffensiveHorse():getId())
				end
				if #select_cards < min_n and self.player:hasTreasure("wooden_ox") and self.player:getPile("wooden_ox"):isEmpty() then
					table.insert(select_cards, self.player:getTreasure():getId())
				end
			end
		end
		if #select_cards > 0 then
			return "$" .. table.concat(select_cards, "+")
		end
	end
	return ""
end

sgs.ai_skill_cardask["@huisheng-obtain"] = function(self, data)
	local damage = data:toDamage()
	local huanghao = damage.to
	local card_ids = self.player:getTag("huisheng_forAI"):toString():split("+")

	local not_optional = (#card_ids > self.player:getCardCount(true))
	local standby = self:askForAG(card_ids, false, "huisheng")
	if self:isFriend(huanghao) then
		if huanghao:getArmor() and self:needToThrowArmor(huanghao) then
			return "$" .. huanghao:getArmor():getId()
		end
	else
		if not_optional or ((self:isValuableCard(sgs.Sanguosha:getCard(standby)) or self.player:hasSkills(sgs.double_slash_skill)) and damage.damage == 1) then
			return "$" .. standby
		end
		local to_discard = self:askForDiscard("dummy", #card_ids, #card_ids, false, true)
		local acceptable = 2
		if self:isWeak(huanghao) then acceptable = math.max(self.player:getCardCount(true) / 3, 3) end
		if #card_ids < acceptable then
			return ""
		end
	end
	return "$" .. standby
end

sgs.ai_skill_choice.guizao = function(self, choices)
	return "recover"
end

















