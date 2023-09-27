#期末專題#
import xtools, utime,urequests,ujson
from machine import  Timer,RTC,Pin
from umqtt.simple import MQTTClient
import ntptime
from machine import Pin
from machine import UART
import utime

#---傳給8051的訊息種類---#
Error = '1'
Win	= '2'
Loss = '3'
Wrong = '4'
Start = '5'
Time = '6'

#---遊戲狀態及種類---#
ply2_account = 1
ply2_answer = 2
waiting = 3
game_start = 4
game_end = 5
check_continue = 6
stage = game_end

#---連網---#
xtools.connect_wifi_led()			

#---UART設定----#
com = UART(1, 9600, tx=17, rx=16)	#傳送端17腳 接收端16腳
com.init(9600)

#---IFTTT設定---#
API_KEY = "d945brOfILN3ic_eCXkuSa"
EVENT_NAME = "game_record"
WEBHOOK_URL="https://maker.ifttt.com/trigger/" + EVENT_NAME
WEBHOOK_URL+="/with/key/" + API_KEY + "/?value1="

#---firebase設定---#
DB = "distance2-1b2df-default-rtdb"
DB_URL = "https://"+DB+".firebaseio.com/1A2B"

#---玩家相關設定與紀錄---#
player1 = None		#玩家一帳號
player2 = None		#玩家二帳號
ply1_ready = 0		#玩家一已傳送帳號
ply2_ready = 0		#玩家二已傳送帳號及題目答案
player1_flag = 0	#收到account指令
guess_flag = 0		#收到guess指令
ply1_record = [0,0]
ply2_record = [0,0]
answer = "1234"		#出題答案
rounds = 0			#回合
rounds_limit = 5	#回合限制
game_time = 300		#遊戲限時300秒

#---檢查答案是否符合規則的函式---#
def check_rule(num):
    #長度需等於4#
    if len(num)!= 4 :
        return False
    #數字需不重複#
    for i in range(4):
        for j in range(i+1,4):
            #重複則回傳不符合規則
            if num[i] == num[j]:
                return False
    return True

#---檢查猜測是否等於答案的函式---#
def check_ans(num):
    for i in range(4):
        if num[i] != answer[i]:
            #不相同則回傳猜錯
            return False
    return True

#---猜錯時判斷幾A幾B的函式---#
def guess_hint(num):
    a = 0
    b = 0
    for i in range(4):
        for j in range(4):
            #數字相同#
            if num[i] == answer[j]:
                if i == j :	#位置也相同則為A
                    a = a + 1
                else:		#位置不同則為B
                    b = b + 1
    res = str(a) + "A" + str(b) + "B"
    return res

#---收到MQTT訊息呼叫的函式---#
def sub_cb(topic,msg):
    global stage,answer
    global player2,ply2_ready
    
    ply2_msg = msg.decode()		#解碼成字串形式
    
    #---若當前狀態在等玩家二的帳號---#
    if stage == ply2_account:	
        #檢查帳號是否符合長度8的規定#
        if len(ply2_msg)!=8:
            client.publish(host_topic, "Please enter your account!\n(8 number)")
        else:
            player2 = ply2_msg
            #符合則將玩家二帳號登入記錄到google表單
            mes = WEBHOOK_URL+"Player2:"+player2+"---Login"
            xtools.webhook_get(mes)
            #狀態轉移到等待玩家二出題
            stage = ply2_answer
            client.publish(host_topic, "Please enter your answer!\n(4 number and number must be different)")
    
    #---若當前狀態在等玩家二出題---#
    elif stage == ply2_answer:
        #檢查出的題是否符合規則#
        if check_rule(ply2_msg):
            #符合則將題目儲存下來並記錄到google表單
            answer = ply2_msg
            mes = WEBHOOK_URL+"Answer:"+answer
            xtools.webhook_get(mes)
            #玩家二準備好，狀態轉換到等待玩家一
            ply2_ready = 1
            stage = waiting   
        else:
            client.publish(host_topic, "Does not comply with the rules!")
            client.publish(host_topic, "Please enter again your answer!\n(4 number and number must be different)")
    
    #---若當前狀態在等玩家二確認是否繼續遊戲---#
    elif stage == check_continue:
        if ply2_msg == "Y" or ply2_msg == "y":
            #若繼續，則等同玩家二輸入帳號時的操作
            stage = ply2_answer
            mes = WEBHOOK_URL+"Player2:"+player2+"---Login"
            xtools.webhook_get(mes)
            client.publish(host_topic, "Please enter your answer!\n(4 number and number must be different)")
        else:
            #不繼續，則狀態轉移到等待玩家二輸入帳號
            mes = WEBHOOK_URL+"Player2:"+ player2+"---Logout."
            xtools.webhook_get(mes)
            player2 = None
            stage = ply2_account
            client.publish(host_topic, "Please enter your account!\n(8 number)")
            
    
#---遊戲結束處理後置作業的函式---#
def game_end_init():
    global stage,rounds,game_time
    global ply1_ready,ply2_ready,player1,player2,winner
    global ply1_record,ply2_record
    global t
    
    #關閉timer，不繼續倒數#
    t.deinit()
    
    #---更新player1戰績到firebase---#
    #讀取player1先前戰績
    mes = DB_URL + "/" + player1 + ".json"
    res = urequests.get(mes)
    r=res.text
    res.close()	#關閉連線避免佔資源
    
    #若有戰績，則將當前結果加上之前戰績
    if r!= "null":
        data = ujson.loads(r)
        ply1_record[0]+=data["win"]
        ply1_record[1]+=data["loss"]

    data = {"win": ply1_record[0],
            "loss": ply1_record[1]}
    mes = DB_URL + "/" + player1 + ".json"
    r = urequests.put(mes,json=data)
    r.close()

    #---更新player2戰績到firebase---#
    #讀取player2先前戰績
    mes = DB_URL + "/" + player2 + ".json"
    res = urequests.get(mes)
    r=res.text
    res.close()	#關閉連線避免佔資源

    #若有戰績，則將當前結果加上之前戰績
    if r!= "null":
        data = ujson.loads(r)
        ply2_record[0]+=data["win"]
        ply2_record[1]+=data["loss"]
    
    data = {"win": ply2_record[0],
            "loss": ply2_record[1]}
    mes = DB_URL + "/" + player2 + ".json"
    r = urequests.put(mes,json=data)
    r.close()

    #---重新初始化遊戲相關參數---#
    ply1_ready = 0
    ply2_ready = 0
    rounds = 0
    game_time = 300
    #狀態轉移到等待玩家二確認是否繼續遊玩
    stage = check_continue
    client.publish(host_topic, "Do you want to continue? [Y/y for yes]")

#---更新遊戲剩餘時間的函式---#
def update_game_time(t):
    global game_time
    game_time = game_time - 1
    #通知8051更新遊戲剩餘時間
    com.write(Time)
 
#---設定Timer來計時---#
t= Timer(0)

#---MQTT相關設定---#
client = MQTTClient(
    client_id = xtools.get_id(),
    server = "broker.hivemq.com",
    ssl = False,
)

client.set_callback(sub_cb)			#收到MQTT訊息呼叫的函式
client.connect()
host_topic = "huahua/1A2B/host"		#在host頻道傳送消息
player_topic = "huahua/1A2B/player"	#player頻道
client.subscribe(player_topic)		#訂閱接收玩家二在player頻道傳送的消息
client.publish(host_topic, "Please enter your account!\n(8 number)")#通知player2傳送帳號	
stage = ply2_account				#更新狀態為等待player2輸入帳號

print('MicroPython Ready...')  # 輸出訊息到終端機
while True:
    
    #---檢查頻道是否有訊息---#
    client.check_msg()
    
    #---若兩位玩家都準備好且未進入遊戲開始狀態---#
    if stage!=game_start and ply1_ready and ply2_ready:
        #轉換狀態到遊戲開始並使用UART通知8051
        stage = game_start
        com.write(Start)
        #開啟timer，每一秒執行更新剩餘時間的函式來達到倒數計時效果
        t.init(period=1000,mode=Timer.PERIODIC,callback=update_game_time)
        client.publish(host_topic, "Game Start!")	#通知玩家二
        #更新遊戲開始的資訊到google表單
        mes = WEBHOOK_URL+"Player1:"+player1+"_Player2:"+player2+"---"+"Game_Start!"
        xtools.webhook_get(mes)
    
    #---若已超過遊戲時間(300秒)，玩家一輸---#    
    if game_time <= 0:
        #告知雙方玩家由遊戲輸贏結果
        com.write(Loss)
        client.publish(host_topic, "Time out.\nCongratulation! You win the game!")
        #呼叫後置處理函式，並上傳遊戲紀錄到google表單
        mes = WEBHOOK_URL+"Time_out!_Player2:"+ player2+"---win_the_game."
        ply1_record = [0,1]	#win,loss
        ply2_record = [1,0]
        game_end_init()
        xtools.webhook_get(mes)
    
    #---若從UART讀到玩家一傳來的訊息---# 
    if com.any() > 0:
        s = com.readline()
        s = s.decode()	#傳來的是byte形式，轉成str
        s = s[:-1]		#刪除換行
        
        #若收到的是帳號指令，代表接下來要傳玩家一的帳號#
        if s=="account":		
            player1_flag = 1
            continue
       #若收到的是猜測指令，代表接下來要傳玩家一的猜測#
        elif s=="guess":
            guess_flag = 1
            continue
        #若收到玩家一的退出遊戲指令#
        elif s=="quit":
            #若當前處於遊戲進行中#
            if stage == game_start:
                #告知玩家二獲勝
                client.publish(host_topic, "Player1_quit!\nCongratulation! You win the game!")
                #呼叫後置處理函式，並上傳遊戲紀錄到google表單
                mes = WEBHOOK_URL+"Player1_quit!_Player2:"+ player2+"---win_the_game."
                ply1_record = [0,1]	#win,loss
                ply2_record = [1,0]
                game_end_init()
                xtools.webhook_get(mes)
            
            #若非處於遊戲進行中#
            else:
                #則上傳此記錄到google表單
                mes = WEBHOOK_URL+"Player1:"+ player1+"---Logout."
                xtools.webhook_get(mes)
                #玩家一狀態轉換為未準備好
                ply1_ready = 0
                player1 = None
        
        #代表現在收到的是玩家一的帳號#        
        if player1_flag:
            player1_flag = 0
            player1 = s
            ply1_ready = 1	#玩家一準備好
            mes = WEBHOOK_URL+"Player1:"+player1+"---Login"
            xtools.webhook_get(mes)
        #代表現在收到的是玩家一的猜測#     
        elif guess_flag:
            guess_flag = 0
            #若猜測的數字符合規則#
            if check_rule(s):
                #告知玩家二玩家一的猜測
                res = "Round "+ str(rounds+1) + "---Player1 guess:"+ str(s)
                client.publish(host_topic, res)
                #若猜對，玩家一獲勝#
                if check_ans(s):
                    #告知雙方玩家由遊戲輸贏結果
                    client.publish(host_topic, "Oh... You loss the game.")
                    com.write(Win)
                    #呼叫後置處理函式，並上傳遊戲紀錄到google表單
                    ply1_record = [1,0]
                    ply2_record = [0,1]
                    mes = WEBHOOK_URL+"Round_"+ str(rounds+1) + "_guess:"+ str(s)+"_Bingo!_Player1:"+ player1+"---win_the_game."
                    game_end_init()
                    xtools.webhook_get(mes)
                #若猜錯#
                else:
                    rounds += 1	#回合數+1
                    #若猜測次數達到限制，玩家一輸#
                    if rounds >= rounds_limit:
                        #告知雙方玩家由遊戲輸贏結果#
                        client.publish(host_topic, "Congratulation! You win the game!")
                        com.write(Loss)
                        ply1_record = [0,1]
                        ply2_record = [1,0]
                        mes = WEBHOOK_URL+"Round_"+ str(rounds) + "_guess:"+ str(s)+"_No_chance!_Player2:"+ player2+"---win_the_game."
                        game_end_init()
                        xtools.webhook_get(mes)

                    else:
                        #計算幾A幾B並回傳給玩家1
                        guess_result = guess_hint(s)
                        com.write(Wrong)
                        com.write(guess_result[0])
                        com.write(guess_result[1])
                        com.write(guess_result[2])
                        com.write(guess_result[3])
                        client.publish(host_topic,guess_result)
                        mes = WEBHOOK_URL+"Round_"+ str(rounds) + "_guess:"+ str(s)+"_Player1_got_wrong_answer:"+ guess_result
                        xtools.webhook_get(mes)
            #若猜測的數字不符合規則#
            else:
                com.write(Error)	#回傳錯誤訊息給8051
    
    utime.sleep(0.1)

