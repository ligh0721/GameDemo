include("WorldCommon.lua")


Stage05 = class(World)
function Stage05:onHeroInit()
	return nil -- ȡ��Ĭ�ϵ�����Ӣ��
end

function Stage05:onInit()
	local me = createUnit(UL.kViking)  -- ����һ����λ��ģ�Ͳ���UL.kViking
	me:setPosition(800, 600)  -- ���õ�λ��λ��
	me:setName("sw0rd")  -- ��������
	me:sayf("Hey, guys! I'm %s!", me:getName())  -- ˵һ��Ƿ��Ļ�

	setPortrait(1, me)  -- ��ͷ�����õ�1��λ�ϣ���Ȼ�ӳ��ó�Ҳ���Բ�����ͷ�񡣡�
	setCtrlUnit(me)  -- ��������Բٿ������λ��
	
end

function Stage05:onTick(dt)
end

Stage05:new():run()
