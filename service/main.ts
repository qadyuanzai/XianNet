export function OnInit(){
    XianNet.info("乌鸦坐飞机");
    const gameBoardServiceId = XianNet.newService("game_board/game_board"); 
    XianNet.send(gameBoardServiceId, "Test", null);
}
