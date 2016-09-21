function SmartAI:sortByNumber(cards, inverse)
    local compare_func = function(a, b)
		if not inverse then return a:getNumber() > b:getNumber() end
		return a:getNumber() < b:getNumber()
    end
    table.sort(cards, compare_func)
end

function canEqual213(cards,selected)
	local x = 13
	local stop = false
	while not stop do
		for k,c in ipairs(cards) do
			if c:getNumber() <= x then
				if c:hasFlag("selected") or c:hasFlag("banned") then continue end
				x = x -c:getNumber()
				if x == 0 then
					table.insert(selected,c)
					c:setFlags("selected")
					return true
				else
					if cards[#cards]:getNumber() > x then
						x = x + c:getNumber()
					elseif k ~= #cards then
						table.insert(selected,c)
						c:setFlags("selected")
					end
				end
				if k == #cards then
					x = x + c:getNumber()
					if #selected > 0 then
						for _,ca in ipairs(cards) do
							if ca:getId() == selected[#selected]:getId() then
								ca:setFlags("-selected")
								ca:setFlags("banned")
								x = x + ca:getNumber()
								table.remove(selected)
								break
							end
						end
					end
				end
			end
		end
		if #cards <= 1 then return false end
		if cards[#cards - 1]:hasFlag("banned") then 
			table.remove(cards,1)
			selected = {}
			for _,c in ipairs(cards) do
				c:setFlags("-selected")
				c:setFlags("-banned")
			end
			x = 13
		end
	end
end

sgs.ai_skill_cardask["@tuifeng-put"] = function(self, data)
	local to_discard = {}
	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)
	for _,c in ipairs(cards) do
		if not (c:isKindOf("Peach") or c:isKindOf("Analeptic")) then
			return "$" .. cards[1]:getEffectiveId()
		end
	end
	return "."
end

local ziyuan_skill = {}
ziyuan_skill.name = "ziyuan"
table.insert(sgs.ai_skills, ziyuan_skill)
ziyuan_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ZiyuanCard") then return nil end
	if #self.friends_noself < 1 then return nil end
	local cards=self.player:getCards("h")
	cards=sgs.QList2Table(cards)
	self:sortByUseValue(cards,true)
	for _,card in ipairs(cards) do
		if card:getNumber()== 13 then
			self.room:setCardFlag(card,"chosen")
			return sgs.Card_Parse("@ZiyuanCard=.")
		end
	end
	self:sortByNumber(cards,false)
	cards[#cards]:setFlags("least")
	local selected = {}
	if canEqual213(cards, selected) then
		for _,p in ipairs(selected) do
			self.room:setCardFlag(p,"chosen")
		end
		return sgs.Card_Parse("@ZiyuanCard=.") 
	end
end


sgs.ai_skill_use_func["ZiyuanCard"] = function(card, use, self)
	self:sort(self.friends_noself, "hp")
	local target
	for _,p in ipairs(self.friends_noself) do
		if not hasManjuanEffect(p) then
			target = p
			break
		end
	end
	local to_give = {}
	for _,c in sgs.qlist(self.player:getHandcards()) do
		if c:hasFlag("chosen") then
			table.insert(to_give,c:getEffectiveId())
		end
	end
	if target and #to_give > 0 then
		use.card = sgs.Card_Parse("@ZiyuanCard="..table.concat(to_give,"+"))
		if use.to then use.to:append(target) end
	end
end

sgs.ai_use_value["ZiyuanCard"] = 10
sgs.ai_use_priority["ZiyuanCard"] = 10
sgs.ai_card_intention.ZiyuanCard = -80

sgs.ai_skill_playerchosen.hongde = function(self, targets)
	return self:findPlayerToDraw(false, 1)
end

sgs.ai_playerchosen_intention.hongde = -30

local dingpan_skill = {}
dingpan_skill.name = "dingpan"
table.insert(sgs.ai_skills, dingpan_skill)
dingpan_skill.getTurnUseCard = function(self)
	if self.player:getMark("#dingpan") < 1 then return nil end
	local can_invoke = false
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:getEquips():length() > 0 then
			can_invoke = true
		end
	end
	if not can_invoke then return nil end
	return sgs.Card_Parse("@DingpanCard=.") 
end

sgs.ai_skill_use_func["DingpanCard"] = function(card, use, self)
	local target
	if self.player:getEquips():length() > 1 and self.player:hasArmorEffect("silver_lion") then
		target = self.player
	end
	self:sort(self.enemies, "defense")
	self.enemies = sgs.reverse(self.enemies)
	if not target then
		for _,p in ipairs(self.enemies) do
			if p:getEquips():length() > 0 and not self:hasSkills("xiaoji|xuanfeng",p) and not (self:hasSkills(sgs.masochism_skill,p) and not self:isWeak(p)) then
				target = p
				break
			end
		end
	end
	if not target then
		for _,p in ipairs(self.friends) do 
			if p:getEquips():length() > 0 and ( self:hasSkills("xiaoji|xuanfeng",p) or  (self:hasSkills(sgs.masochism_skill,p))) and not self:isWeak(p) then
				target = p
				break
			end
		end
	end
	if target then
		use.card = card
		if use.to then
			use.to:append(target)
			if use.to:length() >= 1 then return end
		end
	end		
end

sgs.ai_use_value["DingpanCard"] = 8
sgs.ai_use_priority["DingpanCard"] = 8

sgs.ai_skill_choice.dingpan = function(self, choices, data)
	if (self:hasSkills("xiaoji|xuanfeng") or (self:hasSkills(sgs.masochism_skill))) and not self:isWeak() then
		return "takeback"
	end
	return "disequip"
end

local gushe_skill = {}
gushe_skill.name = "gushe"
table.insert(sgs.ai_skills, gushe_skill)
gushe_skill.getTurnUseCard = function(self)
    if self.player:isKongcheng() or self.player:hasUsed("GusheCard") then return end
    return sgs.Card_Parse("@GusheCard=.")
end

sgs.ai_skill_use_func.GusheCard = function(card, use, self)
    local targets = sgs.SPlayerList()
    self:sort(self.enemies)
    for _,enemy in ipairs(self.enemies) do
        if not enemy:isKongcheng() and not (self:needKongcheng(enemy, true) and enemy:getHandcardNum() <= 2) and not (enemy:hasSkills(sgs.lose_equip_skill) and enemy:hasEquip()) then
            targets:append(enemy)
            if targets:length() == 3 then break end
        end
    end
    if targets:length() > 0 then
        use.to = targets
        use.card = card
        
        local max_card
        local max_number = 0
        local cards = sgs.QList2Table(self.player:getCards("h"))
        self:sortByKeepValue(cards)
        
        local rap_num = self.player:getMark("#rap")
        for _,c in ipairs(cards) do
            local number = c:getNumber()
            if number < rap_num then number = number + rap_num end
            if number > max_number then
                max_card = c
                max_number = number
            end
        end
        if rap_num == 6 and max_number < 10 then 
            use.card = nil
            use.to = sgs.SPlayerList()
        end
        
        self.gushe_card = max_card:getEffectiveId()
    end
end

sgs.ai_skill_discard.gushe = function(self, discard_num, min_num, optional, include_equip)
    if self:needKongcheng() then
        return self:askForDiscard("dummy", discard_num, min_num, optional, include_equip)
    end
	local current = self.room:getCurrent()
    if self:isFriend(current) then
        return {}
    end
    return self:askForDiscard("dummy", discard_num, min_num, optional, include_equip)
end

sgs.ai_skill_pindian.gushe = function(minusecard, self, requestor, maxcard, mincard)
    self.room:writeToConsole(self.player:objectName())
end

sgs.ai_use_value["GusheCard"] = 10
sgs.ai_use_priority["GusheCard"] = 10

sgs.ai_skill_invoke.jici = true