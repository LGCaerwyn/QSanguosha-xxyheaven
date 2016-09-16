






local diy_qixi_skill = {}
diy_qixi_skill.name = "diy_qixi"
table.insert(sgs.ai_skills, diy_qixi_skill)
diy_qixi_skill.getTurnUseCard = function(self,inclusive)
	if self.player:hasFlag("qixi_disable") then return false end
	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)
    local black_card
    self:sortByUseValue(cards,true)
    local has_weapon = false
    for _,card in ipairs(cards)  do
		if card:isKindOf("Weapon") then has_weapon=true end
	end
    for _,card in ipairs(cards)  do
		if card:isBlack() and ((self:getUseValue(card) < sgs.ai_use_value.Dismantlement) or inclusive or self:getOverflow() > 0) then
			local shouldUse = true
            if card:isKindOf("Armor") then
				if not self.player:getArmor() then shouldUse = false
				elseif self.player:hasEquip(card) and not self:needToThrowArmor() then shouldUse = false
				end
			end
            if card:isKindOf("Weapon") then
				if not self.player:getWeapon() then shouldUse = false
				elseif self.player:hasEquip(card) and not has_weapon then shouldUse = false
				end
			end
            if card:isKindOf("Slash") then
				local dummy_use = {isDummy = true}
				if self:getCardsNum("Slash") == 1 then
					self:useBasicCard(card, dummy_use)
					if dummy_use.card then shouldUse = false end
				end
			end
            if self:getUseValue(card) > sgs.ai_use_value.Dismantlement and card:isKindOf("TrickCard") then
				local dummy_use = {isDummy = true}
				self:useTrickCard(card, dummy_use)
				if dummy_use.card then shouldUse = false end
			end
            if shouldUse then
				black_card = card
				break
			end
        end
	end
    if black_card then
		local suit = black_card:getSuitString()
		local number = black_card:getNumberString()
		local card_id = black_card:getEffectiveId()
		local card_str = ("dismantlement:diy_qixi[%s:%s]=%d"):format(suit, number, card_id)
		local dismantlement = sgs.Card_Parse(card_str)
        assert(dismantlement)
        return dismantlement
	else
	    local red_card
	    for _,card in ipairs(cards)  do
		    if card:isRed() and ((self:getUseValue(card) < sgs.ai_use_value.Dismantlement) or inclusive or self:getOverflow() > 0) then
			    local shouldUse = true
                if card:isKindOf("Armor") then
				    if not self.player:getArmor() then shouldUse = false
				    elseif self.player:hasEquip(card) and not self:needToThrowArmor() then shouldUse = false
			    	end
			    end
                if card:isKindOf("Weapon") then
				    if not self.player:getWeapon() then shouldUse = false
				    elseif self.player:hasEquip(card) and not has_weapon then shouldUse = false
				    end
			    end
                if card:isKindOf("Slash") then
				    local dummy_use = {isDummy = true}
				    if self:getCardsNum("Slash") == 1 then
					    self:useBasicCard(card, dummy_use)
					    if dummy_use.card then shouldUse = false end
				    end
			    end
                if self:getUseValue(card) > sgs.ai_use_value.Dismantlement and card:isKindOf("TrickCard") then
				    local dummy_use = {isDummy = true}
				    self:useTrickCard(card, dummy_use)
				    if dummy_use.card then shouldUse = false end
			    end
                if shouldUse then
				    red_card = card
				    break
			    end
            end
	    end
	    if red_card then
		    local suit = red_card:getSuitString()
		    local number = red_card:getNumberString()
		    local card_id = red_card:getEffectiveId()
		    local card_str = ("dismantlement:diy_qixi[%s:%s]=%d"):format(suit, number, card_id)
		    local dismantlement = sgs.Card_Parse(card_str)
            assert(dismantlement)
            return dismantlement
		end
	end
end

sgs.diy_qixi_suit_value = {
	spade = 3.9,
	club = 3.9,
	heart = 3.9,
	diamond = 3.9
}

function sgs.ai_cardneed.diy_qixi(to, card)
	return card
end


sgs.ai_skill_invoke.diy_dangjin = function(self, data)
	local target = data:toPlayer()
	if self:isFriend(target) then
		if self:getOverflow(target) > 2 then return true end
		if self:doNotDiscard(target) then return true end
		return (self:hasSkills(sgs.lose_equip_skill, target) and not target:getEquips():isEmpty())
		  or (self:needToThrowArmor(target) and target:getArmor()) or self:doNotDiscard(target)
	end
	if self:isEnemy(target) then
		if self:doNotDiscard(target) then return false end
		return true
	end
	--self:updateLoyalty(-0.8*sgs.ai_loyalty[target:objectName()],self.player:objectName())
	return true
end



sgs.ai_skill_use["@@chiya_discard"] = function(self, prompt)
    local chenqun = findPlayerByObjectName(self.room, prompt:split(":")[2])
	local player = self.player
    local n = tonumber(prompt:split(":")[4])
	if self:isEnemy(chenqun) then
	    local discard = false
		if self:isWeak(chenqun) and n < 3 then
		    discard = true
		elseif n == 1 then
		    discard = true
		end
		if discard then
		    local card_ids = {}
			local unpreferedCards = {}
			local equips = {}
			local ValuableCard = {}
	        local cards = sgs.QList2Table(self.player:getHandcards())
	        self:sortByUseValue(cards)
	        for _, card in ipairs(cards) do
			    if card:isRed() then
		            if self:isValuableCard(card) then 
			            table.insert(ValuableCard, card:getId())
			        else
					    table.insert(unpreferedCards, card:getId())
					end
				end
	        end
			for _, card in sgs.qlist(player:getEquips()) do
			    if card:isRed() then
			        table.insert(equips, card:getId())
				end
			end
			if #unpreferedCards + #ValuableCard + #equips < n then return "." end
			while #card_ids < n do
			    if #unpreferedCards > 0 then
				    table.insert(card_ids, unpreferedCards[1])
					table.removeOne(unpreferedCards, unpreferedCards[1])
				elseif #equips > 0 then
				    table.insert(card_ids, equips[1])
					table.removeOne(equips, equips[1])
				elseif #ValuableCard > 0 then
				    table.insert(card_ids, ValuableCard[1])
					table.removeOne(ValuableCard, ValuableCard[1])
				else
				    break
				end
			end
			if #card_ids == n then
                return "#chiya_discardCard:"..table.concat(card_ids,"+")..":"
			end
		end
	end
	return "."
end










sgs.ai_skill_invoke.diy_lianli = true


--[[
for i = 0, 99999, 1 do
    sgs.ai_skill_use[tostring(i)] = function(self, prompt, method)
        local card = sgs.Sanguosha:getCard(i)
		if card:getTypeId() == sgs.Card_TypeTrick and not card:isKindOf("Nullification") then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			self:useTrickCard(card, dummy_use)
			if dummy_use.card then
				if dummy_use.to:isEmpty() then
					return dummy_use.card:toString()
				else
					local target_objectname = {}
					for _, p in sgs.qlist(dummy_use.to) do
						table.insert(target_objectname, p:objectName())
					end
					return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
				end
			end
		elseif card:getTypeId() == sgs.Card_TypeEquip then
			local dummy_use = { isDummy = true }
			self:useEquipCard(card, dummy_use)
			if dummy_use.card then
				return dummy_use.card:toString()
			end
		elseif card:getTypeId() == sgs.Card_TypeBasic and not card:isKindOf("Jink") then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			self:useBasicCard(card, dummy_use)
			if dummy_use.card then
				if dummy_use.to:isEmpty() then
					return dummy_use.card:toString()
				else
					local target_objectname = {}
					for _, p in sgs.qlist(dummy_use.to) do
						table.insert(target_objectname, p:objectName())
					end
					return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
				end
			end
		end
    end
end

]]
sgs.ai_skill_invoke.mafei_accept = function(self, data)
	local callback = sgs.ai_skill_choice.jiangchi
	local choice = callback(self, "jiang+chi+cancel")
	if choice == "jiang" then
		return true
	end
	return false
end













