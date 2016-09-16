local tj_jingxuan_skill = {}
tj_jingxuan_skill.name = "tj_jingxuan"
table.insert(sgs.ai_skills, tj_jingxuan_skill)
tj_jingxuan_skill.getTurnUseCard = function(self, inclusive)
	if self.player:hasFlag("jingxuan_effect") then return nil end
	return sgs.Card_Parse("#tj_jingxuanCard:.:")	
end

sgs.ai_skill_use_func["#tj_jingxuanCard"] = function(card, use, self)
	use.card = card
end

sgs.ai_use_value["tj_jingxuanCard"] = 3
sgs.ai_use_priority["tj_jingxuanCard"] = 11

sgs.ai_skill_cardask["@jingxuan-recast"] = function(self, data)
	local card = sgs.Sanguosha:getCard(self.player:getMark("jingxuan_card"))
	local handcards = sgs.QList2Table(self.player:getHandcards())
	if self.player:getPhase() ~= sgs.Player_Play then
		if hasManjuanEffect(self.player) then return "." end
		self:sortByKeepValue(handcards)
		for _, card_ex in ipairs(handcards) do
			if self:getKeepValue(card_ex) < self:getKeepValue(card) and not self:isValuableCard(card_ex) then
				return "$" .. card_ex:getEffectiveId()
			end
		end
	else
		if card:isKindOf("Slash") and not self:slashIsAvailable() then return "." end
		self:sortByUseValue(handcards)
		for _, card_ex in ipairs(handcards) do
			if self:getUseValue(card_ex) < self:getUseValue(card) and not self:isValuableCard(card_ex) then
				return "$" .. card_ex:getEffectiveId()
			end
		end
	end
	return "."
end



sgs.ai_skill_cardask["@kenjian-invoke"] = function(self, data)
	local player = self.player
	local current = self.room:getCurrent()
	if self:isFriend(current) then
	    local cards = player:getTag("kenjian_equips"):toString():split("+")
		
		return "$" .. cards[1]
	end
	return "."
end












