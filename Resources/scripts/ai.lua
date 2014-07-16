if __AI__ then return end
__AI__ = true

-- ConditionFunctions
CF = {}

function CF.checkAround(u, a)
	local eff = a:getEffectiveTypeFlags()
	local r = a:getCastTargetRadius()
	if eff == UnitForce.kEnemy then
		local t = u:getNearestEnemyInRange(r)
		if not t then
			return false
		end
	end
	return true
end

function CF.getAttackingTargetUnit(u, a)
	return u:getAttackingTarget()
end

function CF.getAttackingTargetUnitPoint(u, a)
	local t = u:getAttackingTarget()
	if not t then
		return nil, nil
	end
	return t:getPosition()
end

function CF.getNearestEnemy(u, a)
	local r = a:getCastRange()
	return u:getNearestEnemyInRange(r)
end

function CF.getNearestEnemyPoint(u, a)
	local r = a:getCastRange()
	local t = u:getNearestEnemyInRange(r)
	if not t then
		return nil, nil
	end
	return t:getPosition()
end

function CF.getInjuredUnit(u, a)
	return u:getNearestEffectiveInjuredUnitInRange(a:getCastRange(), a:getEffectiveTypeFlags(), 0.75, 0.0)
end

ACF = {}
ACF["����"] = CF.checkAround
ACF["Ӣ�¼�Ծ"] = CF.checkAround
ACF["����"] = CF.checkAround
ACF["ʥ��֮��"] = CF.checkAround
ACF["��"] = CF.checkAround
ACF["�������"] = CF.checkAround
ACF["Ӣ�¼�Ծ"] = CF.checkAround

ACF["����ذ��"] = CF.getNearestEnemyPoint
ACF["��������"] = CF.getNearestEnemyPoint
ACF["ħ������"] = CF.getNearestEnemyPoint
ACF["��׶��"] = CF.getNearestEnemyPoint
ACF["��������"] = CF.getNearestEnemyPoint


ACF["�籩ս��"] = CF.getAttackingTargetUnit
ACF["���Ƹ�¶"] = CF.getInjuredUnit


LuaAI = class(UnitAI)
function LuaAI:ctor()
    self:sctor()
    self.acts = {}

    self.targetId = nil  -- �����˵Ĺ���Ŀ��
    self.hitAndRun = false  -- hitAndRunģʽ
end

function LuaAI:tryCastSpell(unit, a)
	if not a or unit:isDoingCastingAction() or a:isCoolingDown() then
        return false
    end
    
	local castType = a:getCastTargetType()
	local cf = ACF[a:getName()]
	if not cf then
		return false
	end
	
    if castType == CommandTarget.kNoTarget then
        if cf(unit, a) then
            unit:castSpellWithoutTarget(a)
            return true
        end
    elseif castType == CommandTarget.kUnitTarget then
        local t = cf(unit, a)
        if t then
            unit:castSpellWithTargetUnit(a, t)
            return true
        end
    elseif castType == CommandTarget.kPointTarget then
        local x, y = cf(unit, a)
        if x and y then
            unit:castSpellWithTargetPoint(a, x, y)
            return true
        end
    end
    return false
end

function LuaAI:onUnitTick(unit, dt)
	if unit:isDoingCastingAction() then
		-- ����ʩ��������ʱ��ִ��AI
		return
	end
	
    local atk = unit:getAttackAbility()
	local a = unit:getCastingActiveAbility()
	if a and unit:isDoingAnd(Unit.kCasting + Unit.kObstinate) and atk and a:getId() ~= atk:getId() then
		-- �����ǰ����ǿ��ʩ���У��Ҵ���ʩ���ļ��ܲ��ǹ���
    	return
    end
	
	-- �����ͷ����м���
    for _, a in pairs(self.acts) do
    	self:tryCastSpell(unit, a)
    end
    
	if not atk then
		return
	end
	
    local t = unit:getAttackingTarget()
    if t then
        self.targetId = t:getId()
    end
    
	if atk:getCastRange() > 150 then
		-- ��Զ��Ӣ��
		if t and atk:isCoolingDown() then
			-- ����hitAndRunģʽ�У���ǰ���ڹ���һ��Ŀ�꣬�ҹ���cd�У�������hitAndRunģʽ
			local x0, y0 = t:getPosition()
			local x1, y1 = unit:getPosition()
			local dis = atk:getCastRange()
			if x1 > 1500 or x1 < 500 then
				dis = -400
			end
			--logf("hitAndRun")
			local x, y = getForwardPoint(x0, y0, x1, y1, dis)
			if x < 100 or x > 1900 or y < 100 or y > 1400 then
				x, y = getForwardPoint(x0, y0, x1, y1, -dis)
			end
			x = math.min(2000, math.max(0, x))
			y = math.min(1500, math.max(0, y))
			unit:move(x, y + math.random(-50, 50))
			self.hitAndRun = true
		end
		
		if self.hitAndRun and self.targetId then
			-- ������hitAndRunģʽ��
			local lastTarget = getUnit(self.targetId)
			if lastTarget then
				if not atk:isCoolingDown() then
					-- �ҹ���û��cd
					unit:castSpellWithTargetUnit(atk, self.targetId, false)
					self.hitAndRun = false
				end
			else
				-- ֮ǰ��Ŀ���Ѳ�����
				unit:stop()
				self.targetId = nil
				self.hitAndRun = false
			end
		end
    end
    
end

function LuaAI:onUnitAddActiveAbility(unit, a)
	table.insert(self.acts, a)
end

function LuaAI:onUnitAbilityReady(unit, ability)
    if ability:getId() ~= unit:getAttackAbility():getId() then
        --unit:addBattleTip(ability:getName(), "fonts/Comic Book.fnt", 32, 0, 0, 0)
    end
end
