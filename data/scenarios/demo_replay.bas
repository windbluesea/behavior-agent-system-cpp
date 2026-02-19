# BAS 回放格式：使用 CSV 记录 ENV/ENTITY/FIRE 三类数据
# ENV,时间戳毫秒,可见距离米,天气风险,地形风险
# ENTITY,时间戳毫秒,编号,阵营,类型,x,y,z,速度米每秒,航向角,是否存活,威胁等级
# FIRE,时间戳毫秒,射手编号,目标编号,武器名称,x,y,z

ENV,1000,900,0.2,0.3
ENTITY,1000,F-1,friendly,armor,0,0,0,6,15,1,0.35
ENTITY,1000,F-2,friendly,infantry,-25,-10,0,2,20,1,0.20
ENTITY,1000,H-1,hostile,armor,380,180,0,9,210,1,0.95
ENTITY,1000,H-2,hostile,artillery,-160,140,0,3.5,195,1,0.82
FIRE,950,H-2,F-1,howitzer,-160,140,0

ENV,2000,850,0.2,0.35
ENTITY,2000,F-1,friendly,armor,20,30,0,6,25,1,0.35
ENTITY,2000,F-2,friendly,infantry,-10,15,0,2,30,1,0.20
ENTITY,2000,H-1,hostile,armor,320,220,0,9,220,1,0.95
ENTITY,2000,H-2,hostile,artillery,-180,170,0,3.5,200,1,0.82

ENV,3000,780,0.25,0.4
ENTITY,3000,F-1,friendly,armor,45,55,0,6,35,1,0.35
ENTITY,3000,F-2,friendly,infantry,10,35,0,2,40,0,0.20
ENTITY,3000,H-1,hostile,armor,270,250,0,9,230,0,0.95
ENTITY,3000,H-2,hostile,artillery,-200,180,0,3.5,205,1,0.82
