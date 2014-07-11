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

function CF.getAttackingTarget(u, a)
	return u:getAttackingTarget()
end

function CF.getAttackingTargetPoint(u, a)
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

function CF.getInjuredTargetUnit(u, a)
	return u:getNearestEffectiveInjuredUnitInRange(a:getCastRange(), a:getEffectiveTypeFlags(), 0.75, 0.0)
end

ACF = {}
ACF["����"] = CF.checkAround
ACF["Ӣ�¼�Ծ"] = CF.checkAround
ACF["����"] = CF.checkAround
ACF["ʥ��֮��"] = CF.checkAround

ACF["����ذ��"] = CF.getAttackingTargetPoint
ACF["��������"] = CF.getAttackingTargetPoint
ACF["ħ������"] = CF.getAttackingTargetPoint
ACF["��׶��"] = CF.getAttackingTargetPoint

ACF["���Ƹ�¶"] = CF.getInjuredTargetUnit


LuaAI = class(UnitAI)
function LuaAI:ctor()
    self:sctor()
    self.acts = {}

    self.targetId = nil
    self.canAttack = true
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
    local atk = unit:getAttackAbility()
	
	local a = unit:getCastingActiveAbility()
	if a and not a:isCoolingDown() and unit:isDoingAnd(Unit.kCasting + Unit.kObstinate) then
    	return
    end
	
    for _, a in pairs(self.acts) do
    	self:tryCastSpell(unit, a)
    end
    
    local t = unit:getAttackingTarget()
    if t then
        self.targetId = t:getId()
    end
    
    if t and atk and (atk:isCoolingDown()) then
		--����cd��
        local x0, y0 = t:getPosition()
        local x1, y1 = unit:getPosition()
        local dis = atk:getCastRange()
        if x1 > 1500 or x1 < 500 then
            dis = -400
        end

        local x, y = getForwardPoint(x0, y0, x1, y1, dis)
        unit:move(x, y + math.random(-50, 50))
        self.canAttack = false
    end
    
	if not self.canAttack then
		--���������
		if self.targetId and not getUnit(self.targetId) then
			-- ��ǰĿ���Ѳ�����
			unit:stop()
			self.targetId = nil
		end
	end
	
    
    
    if not t and atk and not atk:isCoolingDown() then
        --������ܹ��������� ���ڿɹ���Ŀ�� �ҹ���û��cd
        unit:castSpellWithTargetUnit(atk, self.targetId)
        self.canAttack = true
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
