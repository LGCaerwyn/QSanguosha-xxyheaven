--贺齐

sgs.ai_skill_invoke.xianxi_discard = function(self, data)
	local prompt = data:toString()
	local source = findPlayerByObjectName(self.room, prompt:split(":")[2])
	return not self:isFriend(source)
end

local qd_chidu_skill={}
qd_chidu_skill.name="qd_chidu"
table.insert(sgs.ai_skills,qd_chidu_skill)
qd_chidu_skill.getTurnUseCard=function(self)
	if self.player:getMark("@chidu_slash") == 0 then return nil end
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)

	self:sortByUseValue(cards,true)

    local jink_card = cards[1]
	if not jink_card then return nil end
	local suit = jink_card:getSuitString()
	local number = jink_card:getNumberString()
	local card_id = jink_card:getEffectiveId()
	local card_str = ("slash:qd_chidu[%s:%s]=%d"):format(suit, number, card_id)
	local slash = sgs.Card_Parse(card_str)
	assert(slash)

	return slash

end

sgs.ai_view_as.qd_chidu = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place == sgs.Player_PlaceHand then
		if player:getMark("@chidu_slash") > 0 then
			return ("slash:qd_chidu[%s:%s]=%d"):format(suit, number, card_id)
		elseif player:getMark("@chidu_jink") > 0 then
			return ("jink:qd_chidu[%s:%s]=%d"):format(suit, number, card_id)
		end
	end
end


--孙乾

--copy by thicket-ai.lua--------------------------------------
--要求：mycards是经过sortByKeepValue排序的--
function DimengIsWorth(self, friend, enemy, mycards, myequips)
	local e_hand1, e_hand2 = enemy:getHandcardNum(), enemy:getHandcardNum() - self:getLeastHandcardNum(enemy)
	local f_hand1, f_hand2 = friend:getHandcardNum(), friend:getHandcardNum() - self:getLeastHandcardNum(friend)
	local e_peach, f_peach = getCardsNum("Peach", enemy), getCardsNum("Peach", friend)
	if e_hand1 < f_hand1 then
		return false
	elseif e_hand2 <= f_hand2 and e_peach <= f_peach then
		return false
	elseif e_peach < f_peach and e_peach < 1 then
		return false
	elseif e_hand1 == f_hand1 and e_hand1 > 0 then
		return friend:hasSkills("tuntian+zaoxian")
	end
	local cardNum = #mycards
	local delt = e_hand1 - f_hand1 --assert: delt>0
	if delt > cardNum then
		return false
	end
	local equipNum = #myequips
	if equipNum > 0 then
		if self.player:hasSkills("xuanfeng|xiaoji|nosxuanfeng") then
			return true
		end
	end
	--now e_hand1>f_hand1 and delt<=cardNum
	local soKeep = 0
	local soUse = 0
	local marker = math.ceil(delt / 2)
	for i=1, delt, 1 do
		local card = mycards[i]
		local keepValue = self:getKeepValue(card)
		if keepValue > 4 then
			soKeep = soKeep + 1
		end
		local useValue = self:getUseValue(card)
		if useValue >= 6 then
			soUse = soUse + 1
		end
	end
	if soKeep > marker then
		return false
	end
	if soUse > marker then
		return false
	end
	return true
end

--缔盟的弃牌策略--
local dimeng_discard = function(self, discard_num, mycards)
	local cards = mycards
	local to_discard = {}

	local aux_func = function(card)
		local place = self.room:getCardPlace(card:getEffectiveId())
		if place == sgs.Player_PlaceEquip then
			if card:isKindOf("SilverLion") and self.player:isWounded() then return -2
			elseif card:isKindOf("OffensiveHorse") then return 1
			elseif card:isKindOf("Weapon") then return 2
			elseif card:isKindOf("DefensiveHorse") then return 3
			elseif card:isKindOf("Armor") then return 4
			end
		elseif self:getUseValue(card) >= 6 then return 3 --使用价值高的牌，如顺手牵羊(9),下调至桃
		elseif self:hasSkills(sgs.lose_equip_skill) then return 5
		else return 0
		end
		return 0
	end

	local compare_func = function(a, b)
		if aux_func(a) ~= aux_func(b) then
			return aux_func(a) < aux_func(b)
		end
		return self:getKeepValue(a) < self:getKeepValue(b)
	end

	table.sort(cards, compare_func)
	for _, card in ipairs(cards) do
		if not self.player:isJilei(card) then table.insert(to_discard, card:getId()) end
		if #to_discard >= discard_num then break end
	end
	if #to_discard ~= discard_num then return {} end
	return to_discard
end
-------------------------------------------------------
sgs.ai_skill_use["@@qd_menghao"] = function(self, prompt)
    local mycards = {}
	local myequips = {}
	local keepaslash
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if not self.player:isJilei(c) then
			local shouldUse
			if not keepaslash and isCard("Slash", c, self.player) then
				local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
				self:useBasicCard(c, dummy_use)
				if dummy_use.card and not dummy_use.to:isEmpty() and (dummy_use.to:length() > 1 or dummy_use.to:first():getHp() <= 1) then
					shouldUse = true
				end
			end
			if not shouldUse then table.insert(mycards, c) end
		end
	end
	for _, c in sgs.qlist(self.player:getEquips()) do
		if not self.player:isJilei(c) then
			table.insert(mycards, c)
			table.insert(myequips, c)
		end
	end
	if #mycards == 0 then return end
	self:sortByKeepValue(mycards) --桃的keepValue是5，useValue是6；顺手牵羊的keepValue是1.9，useValue是9

	self:sort(self.enemies,"handcard")
	local friends = {}
	for _, player in ipairs(self.friends_noself) do
		if not player:hasSkill("manjuan") then
			table.insert(friends, player)
		end
	end
	if #friends == 0 then return end

	self:sort(friends, "defense")
	local function cmp_HandcardNum(a, b)
		local x = a:getHandcardNum() - self:getLeastHandcardNum(a)
		local y = b:getHandcardNum() - self:getLeastHandcardNum(b)
		return x < y
	end
	table.sort(friends, cmp_HandcardNum)

	self:sort(self.enemies, "defense")
	for _,enemy in ipairs(self.enemies) do
		if enemy:hasSkill("manjuan") then
			local e_hand = enemy:getHandcardNum()
			for _, friend in ipairs(friends) do
				local f_peach, f_hand = getCardsNum("Peach", friend), friend:getHandcardNum()
				if (e_hand > f_hand - 1) and (e_hand - f_hand) <= #mycards and (f_hand > 0 or e_hand > 0) and f_peach <= 2 then
					if e_hand == f_hand then
						return "#qd_menghaoCard:.:->"..friend:objectName().."+"..enemy:objectName()
					else
						local discard_num = e_hand - f_hand
						local discards = dimeng_discard(self, discard_num, mycards)
						if #discards > 0 then return "#qd_menghaoCard:"..table.concat(discards,"+")..":->"..friend:objectName().."+"..enemy:objectName() end
					end
				end
			end
		end
	end

	for _, enemy in ipairs(self.enemies) do
		local e_hand = enemy:getHandcardNum()
		for _, friend in ipairs(friends) do
			local f_hand = friend:getHandcardNum()
			if DimengIsWorth(self, friend, enemy, mycards, myequips) and (e_hand > 0 or f_hand > 0) then
				if e_hand == f_hand then
					return "#qd_menghaoCard:.:->"..friend:objectName()..enemy:objectName()
				else
					local discard_num = math.abs(e_hand - f_hand)
					local discards = dimeng_discard(self, discard_num, mycards)
					if #discards > 0 then return "#qd_menghaoCard:"..table.concat(discards,"+")..":->"..friend:objectName().."+"..enemy:objectName() end
				end
				
			end
		end
	end
	return "."	
end

local qd_jieshi_skill = {}
qd_jieshi_skill.name = "qd_jieshi"
table.insert(sgs.ai_skills, qd_jieshi_skill)
qd_jieshi_skill.getTurnUseCard = function(self, inclusive)
	if self.player:hasUsed("#qd_jieshiCard") then return nil end
	return sgs.Card_Parse("#qd_jieshiCard:.:")
end

sgs.ai_skill_use_func["#qd_jieshiCard"] = function(card, use, self)
	local target
	self.jieshichoice = "target"
	if self.player:faceUp() then
	    self:sort(self.friends_noself, "handcard")
	    for _, friend in ipairs(self.friends_noself) do
		    if not friend:faceUp() then
				target = friend
			    break
		    end
		end
		if not target then
		    if self.player:isChained() then
		        if self.player:getHandcardNum() > self.player:getHp() then
			        for _, friend in ipairs(self.friends_noself) do
		                if not friend:isChained() then
				            target = friend
						    self.jieshichoice = "self"
			                break
		                end
		            end
			    end
		    else
		        self:sort(self.friends_noself, "handcard")
			    self.friends_noself = sgs.reverse(self.friends_noself)
	            for _, friend in ipairs(self.friends_noself) do
		            if friend:isChained() and friend:inMyAttackRange(self.player) then
				        target = friend
			            break
		            end
		        end
			    if not target then
			       for _, friend in ipairs(self.friends_noself) do
		                if friend:isChained() then
				            target = friend
			                break
		                end
		            end
			    end
		    end
		end
	else
	    self:sort(self.friends_noself, "handcard")
	    for _, friend in ipairs(self.friends_noself) do
		    if friend:faceUp() and not self:toTurnOver(friend, 1, "qd_jieshi") then
			    target = friend
			    break
		    end
	    end
		if not target then
		    self:sort(self.enemies)
			for _, enemy in ipairs(self.enemies) do
				if self:toTurnOver(enemy, 1, "qd_jieshi") and hasManjuanEffect(enemy) then
					target = enemy
					break
				end
			end
			if not target then
				for _, enemy in ipairs(self.enemies) do
					if self:toTurnOver(enemy, 1, "qd_jieshi") and self:hasSkills(sgs.priority_skill, enemy) then
						target = enemy
						break
					end
				end
			end
			if not target then
				for _, friend in ipairs(self.friends_noself) do
					if friend:faceUp() then
						target = friend
						self.jieshichoice = "self"
						break
					end
				end
			end
			if not target then
				for _, enemy in ipairs(self.enemies) do
					if self:toTurnOver(enemy, 1, "qd_jieshi") then
						target = enemy
						break
					end
				end
			end
		end
	end
	if not target then return end
	use.card = sgs.Card_Parse("#qd_jieshiCard:.:")
	if use.card and use.to then
		use.to:append(target)
	end
end
sgs.ai_use_value["qd_jieshiCard"] = 3
sgs.ai_use_priority["qd_jieshiCard"] = 5

sgs.ai_skill_choice.qd_jieshi = function(self, choices)
	return self.jieshichoice
end

--董昭

sgs.ai_skill_playerchosen.qd_mouwang = function(self, targets)
	local lord = self.room:getLord()
	if self:isEnemy(lord) then
		if (lord:getCardCount(true) < 2 and not hasManjuanEffect(lord)) or lord:hasSkills("tuntian+zaoxian") then return nil end
	end
	self:sort(self.friends)
	for _, friend in ipairs(self.friends) do
	    if friend:objectName() ~=  lord:objectName() then
		    return friend
		end
	end
	return self.player
end

local function lijian_slash(self)
    local slash = sgs.Sanguosha:cloneCard("slash")
	local lord = self.room:getLord()
	if self.role == "rebel" or (self.role == "renegade" and sgs.current_mode_players["loyalist"] + 1 > sgs.current_mode_players["rebel"]) then--反贼方

		if lord and not lord:isNude() and lord:objectName() ~= self.player:objectName() then		-- 优先离间1血忠和主
			self:sort(self.enemies, "defense")
			local e_peaches = 0
			local loyalist

			for _, enemy in ipairs(self.enemies) do
				e_peaches = e_peaches + getCardsNum("Peach", enemy)
				if enemy:getHp() == 1 and self:slashIsEffective(slash, enemy, lord) and enemy:objectName() ~= lord:objectName() then
					loyalist = enemy
					break
				end
			end

			if loyalist and e_peaches < 1 then return lord, loyalist end
		end

		if #self.friends_noself >= 2 and self:getAllPeachNum() < 1 then		--收友方反
			local nextplayerIsEnemy
			local nextp = self.player:getNextAlive()
			for i = 1, self.room:alivePlayerCount() do
				if not self:willSkipPlayPhase(nextp) then
					if not self:isFriend(nextp) then nextplayerIsEnemy = true end
					break
				else
					nextp = nextp:getNextAlive()
				end
			end
			if nextplayerIsEnemy then
				local round = 50
				local to_die, nextfriend
				self:sort(self.enemies, "hp")

				for _, a_friend in ipairs(self.friends_noself) do	-- 目标1：寻找1血友方
					if a_friend:getHp() == 1 and a_friend:isKongcheng() and not self:hasSkills("kongcheng", a_friend) then
						for _, b_friend in ipairs(self.friends_noself) do		--目标2：寻找位于我之后，离我最近的友方
							if b_friend:objectName() ~= a_friend:objectName() and self:playerGetRound(b_friend) < round
							and self:slashIsEffective(slash, a_friend, b_friend) then

								round = self:playerGetRound(b_friend)
								to_die = a_friend
								nextfriend = b_friend

							end
						end
						if to_die and nextfriend then return nextfriend, to_die end
					end
				end
            end
		end
	end
	
	if lord and self:isFriend(lord) and lord:hasSkill("hunzi") and lord:getHp() == 2 and lord:getMark("hunzi") == 0	then--孙策觉醒
		local enemycount = self:getEnemyNumBySeat(self.player, lord)
		local peaches = self:getAllPeachNum()
		if peaches >= enemycount then
			local f_target, e_target
			for _, ap in sgs.qlist(self.room:getOtherPlayers(self.player)) do
				if ap:objectName() ~= lord:objectName() and self:slashIsEffective(slash, lord, ap) then
					if self:hasSkills("moukui", ap) and self:isFriend(ap) and not ap:isLocked(slash) then
						return ap, lord
					elseif self:isFriend(ap) then
						f_target = ap
					else
						e_target = ap
					end
				end
			end
			if f_target or e_target then
				local target
				if f_target and not f_target:isLocked(slash) then
					target = f_target
				elseif e_target and not e_target:isLocked(slash) then
					target = e_target
				end
				if target then
					return target, lord
				end
			end
		end
	end
    
	local shenguanyu = self.room:findPlayerBySkillName("wuhun")--神关羽
	if shenguanyu then
		if self.role == "rebel" and lord and lord:objectName() ~= self.player:objectName() and not lord:hasSkill("jueqing") and self:slashIsEffective(slash, shenguanyu, lord) then
			return lord, shenguanyu
		elseif self:isEnemy(shenguanyu) and #self.enemies >= 2 then
			for _, enemy in ipairs(self.enemies) do
				if enemy:objectName() ~= shenguanyu:objectName() and not enemy:isLocked(slash)
					and self:slashIsEffective(slash, shenguanyu, enemy) then
					return enemy, shenguanyu
				end
			end
		end
	end
	
	self:sort(self.enemies, "defense")
	local males, others = {}, {}
	local first, second
	local zhugeliang_kongcheng, xunyu
	self:sort(self.friends_noself, "defense")
	self.friends_noself = sgs.reverse(self.friends_noself)

	for _, enemy in ipairs(self.enemies) do
		if enemy:hasSkill("kongcheng") and enemy:isKongcheng() then zhugeliang_kongcheng = enemy
		elseif enemy:hasSkill("jieming") then xunyu = enemy
		else
			for _, anotherenemy in ipairs(self.enemies) do
				if anotherenemy:objectName() ~= enemy:objectName() then
					if #males == 0 and self:slashIsEffective(slash, enemy, anotherenemy) then
						if not (enemy:hasSkill("hunzi") and enemy:getMark("hunzi") < 1 and enemy:getHp() == 2) then
							table.insert(males, enemy)
						else
							table.insert(others, enemy)
						end
					end
					if #males == 1 and self:slashIsEffective(slash, males[1], anotherenemy) then
						if not anotherenemy:hasSkills("zhiman") then
							table.insert(males, anotherenemy)
						else
							table.insert(others, anotherenemy)
						end
						if #males >= 2 then break end
					end
				end
			end
		end
		if #males >= 2 then break end
	end

	if #males >= 1 and sgs.ai_role[males[1]:objectName()] == "rebel" and males[1]:getHp() == 1 then
		if lord and self:isFriend(lord) and lord:objectName() ~= males[1]:objectName() and self:slashIsEffective(slash, males[1], lord)
				and not lord:isLocked(slash) and lord:objectName() ~= self.player:objectName() and lord:isAlive() then
			return lord, males[1]
		end

		for _, friend in ipairs(self.friends_noself) do
			if friend:objectName() ~= males[1]:objectName() and not friend:isLocked(slash) and self:slashIsEffective(slash, males[1], friend) then
				return friend, males[1]
			end
		end
	end

	if #males == 1 then
		if isLord(males[1]) and sgs.turncount <= 1 and self.role == "rebel" and self.player:aliveCount() >= 3 then
			local p_slash, max_p, max_pp = 0
			for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
				if not self:isFriend(p) and p:objectName() ~= males[1]:objectName() and self:slashIsEffective(slash, males[1], p) and not p:isLocked(slash) then
					if p:getKingdom() == males[1]:getKingdom() then
						max_p = p
						break
					elseif not max_pp then
						max_pp = p
					end
				end
			end
			if max_p then table.insert(males, max_p) end
			if max_pp and #males == 1 then table.insert(males, max_pp) end
		end
	end

	if #males == 1 then
		if #others >= 1 and not others[1]:isLocked(slash) then
			table.insert(males, others[1])
		elseif xunyu and not xunyu:isLocked(slash) then
			if getCardsNum("Slash", males[1]) < 1 then
				table.insert(males, xunyu)
			else
				local drawcards = 0
				for _, enemy in ipairs(self.enemies) do
					local x = enemy:getMaxHp() > enemy:getHandcardNum() and math.min(5, enemy:getMaxHp() - enemy:getHandcardNum()) or 0
					if x > drawcards then drawcards = x end
				end
				if drawcards <= 2 then
					table.insert(males, xunyu)
				end
			end
		end
	end

	if #males == 1 and #self.friends_noself > 0 then
		self:log("Only 1")
		first = males[1]
		if zhugeliang_kongcheng and self:slashIsEffective(slash, first, zhugeliang_kongcheng) then
			table.insert(males, zhugeliang_kongcheng)
		else
			table.insert(males, self.friends_noself[1])
		end
	end

	if #males >= 2 then
		first = males[1]
		second = males[2]
		if lord and first:getHp() <= 1 then
			if self.player:isLord() or sgs.isRolePredictable() then
			    for _, friend in ipairs(self.friends_noself) do
			        if friend:objectName() ~= first:objectName() and not friend:isLocked(slash) and self:slashIsEffective(slash, first, friend) then
				        second = friend
			        end
			    end
			else
				if self.role=="rebel" and not first:isLord() and self:slashIsEffective(slash, first, lord) then
						second = lord
				else
					if ((self.role == "loyalist" or self.role == "renegade") and not self:hasSkills("ganglie|enyuan|neoganglie|nosenyuan", first)) then
						second = lord
					end
				end
			end
		end

		if first and second and first:objectName() ~= second:objectName() and not second:isLocked(slash) then
			return second, first
		end
	end
end

sgs.ai_skill_use["@@qd_jiaoling"] = function(self, prompt)
    local slash = sgs.Sanguosha:cloneCard("slash")
	local first, second = lijian_slash(self)
	if first and second then
	    if self:isWeak() and self:getCardsNum("Jink") < 2 and self:getCardsNum("Slash") < 1 and first:canSlash(self.player, slash, false) and self:slashIsEffective(slash, first, self.player) then return "." end
	    return "#qd_jiaolingCard:.:->"..first:objectName().."+"..second:objectName()
	end
	return "."	
end


sgs.ai_skill_cardask["@jiaoling-slash"] = function(self, data, pattern, target)
	if self:getDamagedEffects(self.player, target) or self:needToLoseHp(self.player, target) then return "." end
	return self:getCardId("Slash")
end


sgs.ai_skill_playerchosen.qd_dingjiang = function(self, targets)
	local card = sgs.Sanguosha:cloneCard("ex_nihilo")
	self:sort(self.enemies, "hp")
	local target = self.enemies[1]
	local callback = sgs.ai_skill_choice.jiangchi
	local choice = callback(self, "jiang+chi+cancel")
	if choice == "jiang" and self:hasTrickEffective(card, self.player, self.player) and not self:isWeak(target) then
		return nil
	end
	return target
end



















