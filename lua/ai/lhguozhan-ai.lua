--阵法技ai

sgs.ai_skill_invoke.array_accept = function(self, data)
	local prompt = data:toString()
	local source = findPlayerByObjectName(self.room, prompt:split(":")[2])
	local skill = prompt:split(":")[4]
	local target = self.room:getTag("ArraySkillTarget"):toPlayer()
	if skill == "diy_fengshi" then
	    if self:isEnemy(target) then return true end
		if self:isFriend(target) then return false end
		return self:isFriend(source)
	elseif skill == "diy_niaoxiang" then
	    if self:isEnemy(target) then return true end
		if self:isFriend(target) then return false end
		return self:isFriend(source)
	end
end


--张角
sgs.ai_skill_cardask["@xinzu_give"] = function(self, data)
	local source = self.room:findPlayerBySkillName("diy_xinzu")
	if self:isFriend(source) then
	    local has_analeptic, has_slash, has_jink,has_else
	    for _, acard in sgs.qlist(self.player:getHandcards()) do
		    if acard:isKindOf("Analeptic") then has_analeptic = acard
		    elseif acard:isKindOf("Slash") then has_slash = acard
		    elseif acard:isKindOf("Jink") then has_jink = acard
		    else has_else = acard
		    end
	    end

	    local card

	    if has_slash then card = has_slash
	    elseif has_jink then card = has_jink
	    elseif has_else then card = has_else
	    elseif has_analeptic then
		    if (not self:isWeak()) or self:getCardsNum("Analeptic") > 1 then
			    card = has_analeptic
		    end
	    end

	    if not card then return "." end
		return "$" .. card:getEffectiveId()
	else
	    return "."
	end
end



sgs.ai_view_as.jiaozhong = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place ~= sgs.Player_PlaceSpecial and card:getClassName() == "Slash" and not card:hasFlag("using") then
		return ("thunder_slash:jiaozhong[%s:%s]=%d"):format(suit, number, card_id)
	end
end

sgs.ai_skill_invoke.jiaozhong = function(self, data)
	return true
end

local jiaozhong_skill = {}
jiaozhong_skill.name = "jiaozhong"
table.insert(sgs.ai_skills, jiaozhong_skill)
jiaozhong_skill.getTurnUseCard = function(self, inclusive)
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)

	local slash
	self:sortByUseValue(cards, true)
	for _, card in ipairs(cards) do
		if card:getClassName() == "Slash" then
			slash = card
			break
		end
	end

	if not slash then return nil end
	local dummy_use = { to = sgs.SPlayerList(), isDummy = true }
	self:useCardThunderSlash(slash, dummy_use)
	if dummy_use.card and dummy_use.to:length() > 0 then
		local use = sgs.CardUseStruct()
		use.from = self.player
		use.to = dummy_use.to
		use.card = slash
		local data = sgs.QVariant()
		data:setValue(use)
		if not sgs.ai_skill_invoke.jiaozhong(self, data) then return nil end
	else return nil end

	if slash then
		local suit = slash:getSuitString()
		local number = slash:getNumberString()
		local card_id = slash:getEffectiveId()
		local card_str = ("thunder_slash:jiaozhong[%s:%s]=%d"):format(suit, number, card_id)
		local mySlash = sgs.Card_Parse(card_str)

		assert(mySlash)
		return mySlash
	end
end


sgs.ai_skill_playerchosen.diy_hongfa = function(self, targets)
	local good_enemies = {}
	for _, enemy in ipairs(self.enemies) do
		if not enemy:isChained() then
			table.insert(good_enemies, enemy)
		end
	end
	if #good_enemies == 0 then return nil end
	self:sort(good_enemies)
	return good_enemies[1]
end

--刘备
sgs.ai_skill_use["@@zhiyu_give"] = function(self, prompt)
    local card_ids = {}
	local player = self.player
    if player:hasFlag("zhiyu_target") then 
	    local liubei = self.room:getCurrent()
		if self:isFriend(liubei) then
	        local cards = player:getHandcards()
	        cards = sgs.QList2Table(cards)
	        for i=1, #cards do
		        table.insert(card_ids, cards[i]:getId())
	        end
			if self:needToThrowArmor() then
	            table.insert(card_ids, player:getArmor():getId())
			end
			if self:hasSkills(sgs.lose_equip_skill, player) then
		        if player:getWeapon() then card_id = player:getWeapon():getId()
		        elseif player:getOffensiveHorse() then card_id = player:getOffensiveHorse():getId()
		        elseif player:getArmor() then card_id = player:getArmor():getId()
		        elseif player:getDefensiveHorse() then card_id = player:getDefensiveHorse():getId()
		        end
	        end
			if player:getOffensiveHorse() then
			    table.insert(card_ids, player:getOffensiveHorse():getId())
			end
			if player:getWeapon() then
			    table.insert(card_ids, player:getWeapon():getId())
			end
			if player:getTreasure() and player:getPile("wooden_ox"):isEmpty() then
                table.insert(card_ids, player:getTreasure():getId())
			end
			if player:hasSkill("xiaoji") and not self:isWeak() then
			    if player:getArmor() then
			        table.insert(card_ids, player:getArmor():getId())
			    end
				if player:getDefensiveHorse() then
			        table.insert(card_ids, player:getDefensiveHorse():getId())
			    end
			end
			return "#zhiyu_giveCard:"..table.concat(card_ids,"+")..":"
		end
	 
	end
	return "."	
end


--甘夫人

sgs.ai_skill_choice.diy_yujie = function(self, choices)
	local player = self.player
	if player:isWeak() then return "recover" end 
	local ganfuren = self.room:findPlayerBySkillName("diy_yujie")
	if self:isFriend(player) then
	    if ganfuren:isWounded() then
	        return "draw"
	    else
	        return "recover"
	    end
	else
	    if ganfuren:isWounded() then
	        return "recover"
	    else
	        return "draw"
	    end
	end
end



--糜夫人

sgs.ai_skill_invoke.diy_cunsi = function(self, data)
	local player = data:toPlayer()
	return player and self:isFriend(player) and not (self:needKongcheng(player, true) and not self:hasCrossbowEffect(player))
end


sgs.ai_skill_invoke.diy_yicheng = function(self, data)
	local player = data:toPlayer()
	return player and self:isFriend(player) and not self:needKongcheng(player, true)
end


sgs.ai_skill_cardask["@yicheng-jink"] = function(self, data, pattern, target)
	local target = data:toCardUse().from
	if self:isEnemy(target) then
		if self:doNotDiscard(target) then
			return "."
		end
	elseif self:isFriend(target) then
		if not (self:needToThrowArmor(target) or self:doNotDiscard(target)) then
		    return "."
		end
	end
end


--孔融

sgs.ai_skill_cardask["@mingshi-discard"] = function(self, data)
	local damage = data:toDamage()
	if not self:isFriend(damage.to) then return "." end
	local to_discard = self:askForDiscard("diy_mingshi", 1, 1, false, true)
	if #to_discard > 0 then return "$" .. to_discard[1] else return "." end
end

function sgs.ai_cardneed.diy_mingshi(to, card)
	return to:getCardCount() <= 2
end


--马腾
sgs.ai_skill_discard.diy_langqin = function(self, discard_num, min_num, optional, include_equip)
	local discard = {}
	local player = self.player
	local mateng = self.room:getCurrent()
	local select_armor = false
	if self:needToThrowArmor() then
	    table.insert(discard, player:getArmor():getEffectiveId())
		select_armor = true
	end
	if self:hasSkills(sgs.lose_equip_skill, player) then
		if player:hasSkill("xiaoji") then
		    if player:getWeapon() then 
		        table.insert(discard, player:getWeapon():getEffectiveId())
		    end
		    if player:getOffensiveHorse() then 
		        table.insert(discard, player:getOffensiveHorse():getEffectiveId())
		    end
		    if player:getArmor() and not select_armor and not self:isWeak() then 
		        table.insert(discard, player:getArmor():getEffectiveId())
		    end
		    if player:getDefensiveHorse() and not self:isWeak() then 
		        table.insert(discard, player:getDefensiveHorse():getEffectiveId())
		    end
		elseif not select_armor then
		    if player:getWeapon() then table.insert(discard, player:getWeapon():getEffectiveId())
		    elseif player:getOffensiveHorse() then table.insert(discard, player:getOffensiveHorse():getEffectiveId())
		    elseif player:getArmor() and not self:isWeak() then table.insert(discard, player:getArmor():getEffectiveId())
		    elseif player:getDefensiveHorse() and not self:isWeak() then table.insert(discard, player:getDefensiveHorse():getEffectiveId())
		    end
		end
	end
	if not self:isFriend(mateng) and self:damageIsEffective(player, sgs.DamageStruct_Normal, mateng) and not self:getDamagedEffects(player, mateng) and not self:needToLoseHp(player, mateng) then
	    local cards = sgs.QList2Table(player:getHandcards())
		local cardnum, peachnum, basicnum, othernum = #cards, 0, 0, 0
		for _, card in ipairs(cards) do
			if card:isKindOf("Peach") or card:isKindOf("Analeptic") then
				peachnum = peachnum + 1
			end
			if card:getTypeId() == sgs.Card_TypeBasic then
				basicnum = basicnum + 1
			else
				othernum = othernum + 1
			end
		end
		if cardnum == 1 and basicnum == 1 then
		    table.insert(discard, cards[1]:getId())
		else
		    if self:isWeak() then
		        if peachnum == 0 and not player:hasSkill("jijiu") then
			        if cardnum == basicnum and cardnum < 4 then
		                for _, c in ipairs(cards) do
						    table.insert(discard, c:getId())
						end
					end
			        if basicnum < 3 then
					    self:sortByKeepValue(cards)
						for _, c in ipairs(cards) do
						    if c:getTypeId() == sgs.Card_TypeBasic then
						        table.insert(discard, c:getId())
							end
						end
					end
				else
				    if peachnum == 1 and  cardnum < 3 then
					    for _, c in ipairs(cards) do
						    if c:getTypeId() == sgs.Card_TypeBasic then
						        table.insert(discard, c:getId())
							end
						end
					end
			    end
		    end
		end
	end
	return discard
end

















--于吉
sgs.ai_skill_cardask["@qianhuan-put"] = function(self, data, pattern, target)
	local yuji = self.room:findPlayerBySkillName("diy_qianhuan")
	if not self:isFriend(yuji) then return "." end
	local suits = pattern:split("|")[2]:split(",")
	if self:needToThrowArmor() and table.contains(suits, self.player:getArmor():getSuitString()) then
	    return "$" .. self.player:getArmor():getEffectiveId()
	end
	local card_id
	if self:hasSkills(sgs.lose_equip_skill, self.player) then
	    if self.player:getWeapon() and table.contains(suits, self.player:getWeapon():getSuitString()) then
		    card_id = player:getWeapon():getId()
		elseif self.player:getOffensiveHorse() and table.contains(suits, self.player:getOffensiveHorse():getSuitString()) then
		    card_id = player:getOffensiveHorse():getId()
		elseif self.player:getArmor() and table.contains(suits, self.player:getArmor():getSuitString()) then 
		    card_id = player:getArmor():getId()
		elseif self.player:getDefensiveHorse() and table.contains(suits, self.player:getDefensiveHorse():getSuitString()) then 
		    card_id = player:getDefensiveHorse():getId()
		end
	end
	if card_id then return "$" .. card_id end
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	for _, c in ipairs(cards) do
		if table.contains(suits, c:getSuitString()) then return "$" .. c:getEffectiveId() end
	end
	return "."
end




