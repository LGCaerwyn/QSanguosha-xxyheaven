function SmartAI:useCardAllArmy(card, use)
	local enemies = self:exclude(self.enemies, card)

	local zhanghe = self.room:findPlayerBySkillName("qiaobian")
	local zhanghe_seat = zhanghe and zhanghe:faceUp() and not zhanghe:isKongcheng() and not self:isFriend(zhanghe) and zhanghe:getSeat() or 0

	local sb_daqiao = self.room:findPlayerBySkillName("yanxiao")
	local yanxiao = sb_daqiao and not self:isFriend(sb_daqiao) and sb_daqiao:faceUp() and
					(getKnownCard(sb_daqiao, self.player, "diamond", nil, "he") > 0
					or sb_daqiao:getHandcardNum() + self:ImitateResult_DrawNCards(sb_daqiao, sb_daqiao:getVisibleSkillList(true)) > 3
					or sb_daqiao:containsTrick("YanxiaoCard"))

	if #enemies == 0 then return end

	local getvalue = function(enemy)
		if enemy:containsTrick("all_army") or enemy:containsTrick("YanxiaoCard") then return -100 end
		if enemy:getMark("juao") > 0 then return -100 end
		if enemy:hasSkill("qiaobian") and not enemy:containsTrick("all_army") and not enemy:containsTrick("indulgence") then return -100 end
		if zhanghe_seat > 0 and (self:playerGetRound(zhanghe) <= self:playerGetRound(enemy) and self:enemiesContainsTrick() <= 1 or not enemy:faceUp()) then
			return - 100 end
		if yanxiao and (self:playerGetRound(sb_daqiao) <= self:playerGetRound(enemy) and self:enemiesContainsTrick(true) <= 1 or not enemy:faceUp()) then
			return -100 end

		local value = 0 - enemy:getHandcardNum()

		if self:hasSkills("yongsi|haoshi|tuxi|noslijian|lijian|fanjian|neofanjian|dimeng|jijiu|jieyin|manjuan|beige",enemy)
		  or (enemy:hasSkill("zaiqi") and enemy:getLostHp() > 1)
			then value = value + 10
		end
		if self:hasSkills(sgs.cardneed_skill,enemy) or self:hasSkills("zhaolie|tianxiang|qinyin|yanxiao|zhaoxin|toudu|renjie",enemy)
			then value = value + 5
		end
		if self:hasSkills("yingzi|shelie|xuanhuo|buyi|jujian|jiangchi|mizhao|hongyuan|chongzhen|duoshi",enemy) then value = value + 1 end
		if enemy:hasSkill("zishou") then value = value + enemy:getLostHp() end
		if self:isWeak(enemy) then value = value + 5 end
		if enemy:isLord() then value = value + 3 end

		if self:objectiveLevel(enemy) < 3 then value = value - 10 end
		if not enemy:faceUp() then value = value - 10 end
		if self:hasSkills("keji|shensu|qingyi", enemy) then value = value - enemy:getHandcardNum() end
		if self:hasSkills("guanxing|xiuluo|tiandu|guidao|noszhenlie", enemy) then value = value - 5 end
		if not sgs.isGoodTarget(enemy, self.enemies, self) then value = value - 1 end
		if self:needKongcheng(enemy) then value = value - 1 end
		if enemy:getMark("@kuiwei") > 0 then value = value - 2 end
		return value
	end

	local cmp = function(a,b)
		return getvalue(a) > getvalue(b)
	end

	table.sort(enemies, cmp)

	local target = enemies[1]
	if getvalue(target) > -100 then
		use.card = card
		if use.to then use.to:append(target) end
		return
	end
end

sgs.ai_use_value.AllArmy = 7
sgs.ai_keep_value.AllArmy = 3.48
sgs.ai_use_priority.AllArmy = 0.5
sgs.ai_card_intention.AllArmy = 50

sgs.dynamic_value.control_usecard.AllArmy = true


function SmartAI:useCardIncreaseArmy(card, use)
	local friends = self:exclude(self.friends, card)
    self:sort(friends)
    for _,friend in ipairs(friends) do
        if not self:needKongcheng(friend) then
            use.card = card
            if use.to then use.to:append(friend) end
            return
        end
    end
end

sgs.ai_use_value.IncreaseArmy = 7
sgs.ai_keep_value.IncreaseArmy = 4
sgs.ai_use_priority.IncreaseArmy = 7
sgs.ai_card_intention.IncreaseArmy = -120

sgs.dynamic_value.control_usecard.IncreaseArmy = true


function SmartAI:useCardBeatAnother(card, use)
    local enemies = self:exclude(self.enemies, card)
    self:sort(enemies)
    if not use.to then use.to = sgs.SPlayerList() end
    for _,enemy in ipairs(enemies) do
        if self.player:distanceTo(enemy) == 1 and not self:needKongcheng(enemy, true) and not self:hasSkills(sgs.lose_equip_skill, enemy) and (enemy:getCards("he"):length() > 0 and enemy:getCards("he"):length() <= 2) then
            use.card = card
            use.to:append(enemy)
            break
        end
    end
    
    self:sort(self.friends_noself)
    if use.to:length() == 0 then
        for _,friend in ipairs(self.friends_noself) do
            if self.player:distanceTo(friend) == 1 and self:hasSkills(sgs.lose_equip_skill, friend) or (self:needKongcheng(friend, true) and friend:getCards("he"):length() > 0) then
                use.to:append(friend)
                break
            end
        end
    end
    
    
    for _,friend in ipairs(self.friends_noself) do
        if not self:needKongcheng(friend) then
            use.to:append(friend)
            break
        end
    end
    if use.card and use.to and use.to:length() == 2 then
        return
    end
    use.card = nil
    return
end


sgs.ai_use_value.BeatAnother = 7
sgs.ai_keep_value.BeatAnother = 3
sgs.ai_use_priority.BeatAnother = 7
sgs.ai_card_intention.BeatAnother = 60

sgs.dynamic_value.control_usecard.BeatAnother = true

function getHpNum(players, n)
    local total = 0
    for _,p in ipairs(players) do
        if p:getHp() == n then
            total = total + 1
        end
    end
    return total
end

sgs.ai_skill_invoke.xunzhi = function(self)
    local now = getHpNum(self.player:getHp())
    local after = getHpNum(self.player:getHp() - 1)
    if after >= now then return true end
    return false
end