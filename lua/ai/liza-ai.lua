sgs.ai_skill_invoke.zhuanchong_choice = function(self, data)
	local prompt = data:toString()
	local nvwang = findPlayerByObjectName(self.room, prompt:split(":")[2])
	return self:isFriend(nvwang)
end
