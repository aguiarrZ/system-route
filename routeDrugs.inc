#include <a_samp>
#include <streamer>
#include <sscanf2>
#include <zcmd>

/*
                             oooooo     oooo       .o.       ooooooooo.   ooooo       .o.       oooooooooo.  ooooo        oooooooooooo  .oooooo..o 
                              `888.     .8'       .888.      `888   `Y88. `888'      .888.      `888'   `Y8b `888'        `888'     `8 d8P'    `Y8 
                               `888.   .8'       .8"888.      888   .d88'  888      .8"888.      888     888  888          888         Y88bo.      
                                `888. .8'       .8' `888.     888ooo88P'   888     .8' `888.     888oooo888'  888          888oooo8     `"Y8888o.  
                                 `888.8'       .88ooo8888.    888`88b.     888    .88ooo8888.    888    `88b  888          888    "         `"Y88b 
                                  `888'       .8'     `888.   888  `88b.   888   .8'     `888.   888    .88P  888       o  888       o oo     .d8P 
                                   `8'       o88o     o8888o o888o  o888o o888o o88o     o8888o o888bood8P'  o888ooooood8 o888ooooood8 8""88888P'  
*/
new STREAMER_TAG_ACTOR:drugRouteActor[MAX_PLAYERS],
    bool:isDeliveringDrugs[MAX_PLAYERS],
    currentDeliveryID[MAX_PLAYERS],
    dirtyMoney[MAX_PLAYERS],
    Materials[MAX_PLAYERS],
    deliveriesCompleted[MAX_PLAYERS],
    bool:routesActive[MAX_PLAYERS],
    totalActiveRoutes[MAX_PLAYERS]
;

new Float:drugNPCPositions[][4] = {
    {2467.3906, -1633.4247, 13.4056, 356.0593},
    {2315.3347, -1693.2113, 13.5460, 187.2498},
    {2027.5203, -1696.4476, 13.5469, 261.9314},
    {1644.1028, -1692.8269, 20.4594, 178.7348},
    {2720.9341, -2400.1030, 13.6328, 267.5546},
    {2822.5789, -1208.8588, 24.7416, 314.1005},
    {2116.3076, -1049.7256, 26.4148, 212.1937},
    {2582.5654, -1107.2729, 66.6639, 75.0276},
    {1966.1925, -1238.4110, 20.0674, 7.6131},
    {2779.0620, -1515.6509, 24.2201, 273.3834},
    {2238.4182, -1906.1273, 14.9375, 356.6339},
    {1804.8534, -2062.7151, 13.5620, 181.3661},
    {2092.9175, -2077.9363, 13.5469, 175.3500},
    {2758.9089, -1942.6245, 13.5510, 278.9851},
    {2306.0437, -1542.1000, 25.3565, 277.2196},
    {2188.3818, -1281.5485, 24.5767, 218.5021},
    {1954.4404, -1074.8894, 24.7967, 265.6671}
};
new bool:completedDeliveries[MAX_PLAYERS][sizeof(drugNPCPositions)];

new gangsterSkins[][1] = {
    {2},
    {19},
    {29},
    {30},
    {292},
    {293},
    {122}
};

forward nextDeliveri(playerid);
forward debugActorAnimation(actor);
forward processDeliveryComplete(playerid);

/*
                              ooooooooo.   ooooo     ooo oooooooooo.  ooooo        ooooo   .oooooo.    .oooooo..o 
                              `888   `Y88. `888'     `8' `888'   `Y8b `888'        `888'  d8P'  `Y8b  d8P'    `Y8 
                               888   .d88'  888       8   888     888  888          888  888          Y88bo.      
                               888ooo88P'   888       8   888oooo888'  888          888  888           `"Y8888o.  
                               888          888       8   888    `88b  888          888  888               `"Y88b 
                               888          `88.    .8'   888    .88P  888       o  888  `88b    ooo  oo     .d8P 
                              o888o           `YbodP'    o888bood8P'  o888ooooood8 o888o  `Y8bood8P'  8""88888P'  
*/

public OnGameModeInit()
{
    AddStaticVehicle(468, 2514.1953, -1687.9406, 13.2455, 51.1098, 3, 3);

    CreateDynamicActor(179, 2514.3501, -1691.2926, 14.0460, 50.7638);
    CreateDynamic3DTextLabel("{6A5ACD}Rotas de Drogas\n{BEBEBE}Pressione 'Y' para iniciar", -1, 2514.3501, -1691.2926, 14.0460, 10.0);

    SetTimer("DynamicActorArea", 900, true);
    return true;
}

public OnPlayerConnect(playerid)
{
    currentDeliveryID[playerid] = -1;
    isDeliveringDrugs[playerid] = false;
    deliveriesCompleted[playerid] = 0;
    routesActive[playerid] = false;
    totalActiveRoutes[playerid] = 0;
    drugRouteActor[playerid] = INVALID_ACTOR_ID;

    for (new i = 0; i < sizeof(drugNPCPositions); i++)
        completedDeliveries[playerid][i] = false;

    return true;
}

public OnPlayerDisconnect(playerid, reason)
{
    currentDeliveryID[playerid] = -1;
    isDeliveringDrugs[playerid] = false;
    routesActive[playerid] = false;
    totalActiveRoutes[playerid] = 0;

    if (IsValidDynamicActor(drugRouteActor[playerid]))
    {
        DestroyDynamicActor(drugRouteActor[playerid]);
        drugRouteActor[playerid] = INVALID_ACTOR_ID;
    }

    DisablePlayerCheckpoint(playerid);
    return true;
}

public OnPlayerKeyStateChange(playerid, newkeys, oldkeys)
{
    if (newkeys & KEY_YES)
    {
        if (IsPlayerInRangeOfPoint(playerid, 2.0, 2514.3501, -1691.2926, 14.0460))
        {
            startDrugRoutes(playerid);
        }
    }
    return true;
}

public OnPlayerEnterCheckpoint(playerid)
{
    if (isDeliveringDrugs[playerid] && currentDeliveryID[playerid] != -1)
    {
        if (IsPlayerInRangeOfPoint(playerid, 2.0, drugNPCPositions[currentDeliveryID[playerid]][0],
			drugNPCPositions[currentDeliveryID[playerid]][1], drugNPCPositions[currentDeliveryID[playerid]][2]))
	{
            startDeliveryAnimation(playerid);
        }
    }
    return true;
}

public nextDeliveri(playerid)
{
    if (!routesActive[playerid]) return false;

    new nextDeliveryy = selectNextDelivery(playerid);

    if (nextDeliveryy == -1)
    {
        routesActive[playerid] = false;
        isDeliveringDrugs[playerid] = false;
        totalActiveRoutes[playerid] = 0;
        return false;
    }

    createDelivery(playerid, nextDeliveryy);
    return true;
}

public debugActorAnimation(actor)
{
    if (IsValidDynamicActor(actor))
        ApplyDynamicActorAnimation(actor, "SMOKING", "M_SMK_IN", 4.1, true, false, false, true, 1);

    return true;
}

public processDeliveryComplete(playerid)
{
    if (!isDeliveringDrugs[playerid] || currentDeliveryID[playerid] == -1)
        return false;

    ClearAnimations(playerid);

    if (IsValidDynamicActor(drugRouteActor[playerid]))
    {
        DestroyDynamicActor(drugRouteActor[playerid]);
        drugRouteActor[playerid] = INVALID_ACTOR_ID;
    }

    completedDeliveries[playerid][currentDeliveryID[playerid]] = true;
    deliveriesCompleted[playerid]++;

    if (deliveriesCompleted[playerid] >= totalActiveRoutes[playerid])
    {
        new moneyReward = RandomEx(1100, 2000);
        new materialReward = RandomEx(20, 35);

        dirtyMoney[playerid] += moneyReward;
        Materials[playerid] += materialReward;

        new str[200];
        format(str, sizeof str, "Sucesso: {CCCCCC}Voce completou TODAS as %d rotas! Recebeu R$%d de dinheiro sujo e %d materiais.",
            totalActiveRoutes[playerid], moneyReward, materialReward);

        SendClientMessage(playerid, 0x32CD32FF, str);

        deliveriesCompleted[playerid] = 0;
        totalActiveRoutes[playerid] = 0;

        for (new i = 0; i < sizeof(drugNPCPositions); i++)
            completedDeliveries[playerid][i] = false;

        isDeliveringDrugs[playerid] = false;
        routesActive[playerid] = false;
    }
    else
    {
        new moneyReward = RandomEx(500, 1100);
        new materialReward = RandomEx(5, 15);

        dirtyMoney[playerid] += moneyReward;
        Materials[playerid] += materialReward;

        new str[200];
        format(str, sizeof str, "Sucesso: {CCCCCC}Entrega %d/%d concluida! Voce recebeu R$%d de dinheiro sujo e %d materiais.",
            deliveriesCompleted[playerid], totalActiveRoutes[playerid], moneyReward, materialReward);

        SendClientMessage(playerid, 0x32CD32FF, str);

        SetTimerEx("nextDeliveri", 2000, false, "i", playerid);
    }

    currentDeliveryID[playerid] = -1;
    return true;
}

/*
                              .oooooo..o ooooooooooooo   .oooooo.     .oooooo.   oooo    oooo  .oooooo..o 
                             d8P'    `Y8 8'   888   `8  d8P'  `Y8b   d8P'  `Y8b  `888   .8P'  d8P'    `Y8 
                             Y88bo.           888      888      888 888           888  d8'    Y88bo.      
                              `"Y8888o.       888      888      888 888           88888[       `"Y8888o.  
                                  `"Y88b      888      888      888 888           888`88b.         `"Y88b 
                             oo     .d8P      888      `88b    d88' `88b    ooo   888  `88b.  oo     .d8P 
                             8""88888P'      o888o      `Y8bood8P'   `Y8bood8P'  o888o  o888o 8""88888P'  
*/

stock RandomEx(min, max) return random(max - min) + min;

stock startDeliveryAnimation(playerid)
{
    if (!isDeliveringDrugs[playerid] || currentDeliveryID[playerid] == -1)
        return false;

    if (IsPlayerInAnyVehicle(playerid))
        return SendClientMessage(playerid, 0xFF3030FF, "Erro: {CCCCCC}Saia do veiculo para completar a entrega.");

    ApplyAnimation(playerid, "DEALER", "shop_pay", 4.1, 0, 0, 0, 0, 0, 1);

    if (IsValidDynamicActor(drugRouteActor[playerid]))
        ApplyDynamicActorAnimation(drugRouteActor[playerid], "DEALER", "shop_pay", 4.1, 0, 0, 0, 0, 0);

    SetTimerEx("processDeliveryComplete", 4500, false, "i", playerid);

    DisablePlayerCheckpoint(playerid);

    return true;
}

stock startDrugRoutes(playerid)
{
    if (isDeliveringDrugs[playerid])
        return SendClientMessage(playerid, 0xFF3030FF, "Erro: {CCCCCC}Voce ja esta fazendo uma rota de drogas.");

    if (routesActive[playerid])
        return SendClientMessage(playerid, 0xFF3030FF, "Erro: {CCCCCC}Voce ja iniciou as rotas, conclua antes de iniciar novas.");

    totalActiveRoutes[playerid] = RandomEx(4, 9);
    routesActive[playerid] = true;
    deliveriesCompleted[playerid] = 0;

    for (new i = 0; i < sizeof(drugNPCPositions); i++)
        completedDeliveries[playerid][i] = false;

    new nextDelivery = selectNextDelivery(playerid);

    if (nextDelivery == -1)
    {
        routesActive[playerid] = false;
        totalActiveRoutes[playerid] = 0;
        return SendClientMessage(playerid, 0xFF3030FF, "Erro: {CCCCCC}Nenhuma entrega disponivel no momento.");
    }

    createDelivery(playerid, nextDelivery);

    new str[128];
    format(str, sizeof str, "Sucesso: {CCCCCC}Rotas iniciadas! Complete %d entregas para receber a recompensa final.",
        totalActiveRoutes[playerid]);
    SendClientMessage(playerid, 0x32CD32FF, str);

    return true;
}

stock selectNextDelivery(playerid)
{
    new available[sizeof(drugNPCPositions)];
    new count = 0;

    for (new i = 0; i < sizeof(drugNPCPositions); i++)
        if (!completedDeliveries[playerid][i])
            available[count++] = i;

    if (count == 0) return -1;

    return available[random(count)];
}

stock createDelivery(playerid, deliveryId)
{
    new skinIndex = random(sizeof(gangsterSkins));

    currentDeliveryID[playerid] = deliveryId;
    isDeliveringDrugs[playerid] = true;

    drugRouteActor[playerid] = CreateDynamicActor(gangsterSkins[skinIndex][0], drugNPCPositions[deliveryId][0],
		drugNPCPositions[deliveryId][1], drugNPCPositions[deliveryId][2], drugNPCPositions[deliveryId][3]);

    ApplyDynamicActorAnimation(drugRouteActor[playerid],"SMOKING", "M_SMK_IN", 4.1, true, false, false, true, 1);

    new str[128];
    format(str, sizeof str, "Entrega: {CCCCCC}Va ate o checkpoint. Progresso: %d/%d",
        deliveriesCompleted[playerid], totalActiveRoutes[playerid]);
    SendClientMessage(playerid, 0xFFFF00FF, str);

    SetTimerEx("debugActorAnimation", 150, false, "i", drugRouteActor[playerid]);

    SetPlayerCheckpoint(playerid, drugNPCPositions[deliveryId][0],
		drugNPCPositions[deliveryId][1], drugNPCPositions[deliveryId][2], 2.0);

    return true;
}

/*
                               .oooooo.     .oooooo.   ooo        ooooo       .o.       ooooo      ooo oooooooooo.     .oooooo.   
                              d8P'  `Y8b   d8P'  `Y8b  `88.       .888'      .888.      `888b.     `8' `888'   `Y8b   d8P'  `Y8b  
                             888          888      888  888b     d'888      .8"888.      8 `88b.    8   888      888 888      888 
                             888          888      888  8 Y88. .P  888     .8' `888.     8   `88b.  8   888      888 888      888 
                             888          888      888  8  `888'   888    .88ooo8888.    8     `88b.8   888      888 888      888 
                             `88b    ooo  `88b    d88'  8    Y     888   .8'     `888.   8       `888   888     d88' `88b    d88' 
                              `Y8bood8P'   `Y8bood8P'  o8o        o888o o88o     o8888o o8o        `8  o888bood8P'    `Y8bood8P'
*/

COMMAND:irrota(playerid, params[])
{
    SetPlayerPos(playerid, 2506.9316, -1684.8312, 13.5565);
    SetPlayerFacingAngle(playerid, 227.1068);
    return true;
}
