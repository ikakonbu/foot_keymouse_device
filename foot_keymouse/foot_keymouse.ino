// TODO

#include <Mouse.h>
#include <Keyboard.h>

//キー入力用の定数
  #define MOUSEPIN_X 0
  #define MOUSEPIN_Y 2
  #define KBD_1  2
  #define KBD_2  3
  #define KBD_3  4
  #define KBD_4  5
  #define KBD_E  6
  #define SOUND_PIN         9
  #define ENTER_KEY_PIN     14
  #define MOUSEWHEEL_PIN    13
  #define FUNC_KEY_PIN      9
  #define MODE_CHANGE_PIN   12
  #define ARROW_KEY_PIN     11
  #define KEYCODE_ENTER     176
  #define KEYCODE_BACKSPACE 178
  #define KEYCODE_SPACE     32
  #define KEYCODE_CTRL      128
  #define KEYCODE_ARROW_LEFT  216
  #define KEYCODE_ARROW_RIGHT 215
  #define KEYCODE_ARROW_UP    218
  #define KEYCODE_ARROW_DOWN  217
// 定数宣言終わり

//かな入力キーマップ
const String kana_keymap[3][12] = {
  {"", "k", "s", "t", "n", "h", "m", "", "r", "", "t", "y"},
  {"l", "g", "z", "d", "", "b",  "",  "", "",  "", "lt", "ly"},
  {"",  "",  "",  "", "", "p",  "",  "", "",  "", "d", ""}
};
const int keymaplength[12]   = {2,2,2,2,1,3,1,1,1,1,3,2};
const int keymapsiinlist[12] = {0,0,0,0,0,0,0,1,0,2,0,0};
const String siin[3][5] = {
  {"a", "i", "u", "e", "o"},
  {"ya", "]", "yu", ":", "yo"},
  {"wa", "wo", "nn", "-", "?"}
};

//英語入力用キーマップ(前半が普通入力、後半がShift入力)
const String alp_keymap[20][5] ={
  {"@","#","/","&","1"},
  {"a","b","c","","2"},
  {"d","e","f","","3"},
  {"g","h","i","","4"},
  {"j","k","l","","5"},
  {"m","n","o","","6"},
  {"p","q","r","s","7"},
  {"t","u","v","","8"},
  {"w","x","y","z","9"},
  {"\"","\'","(",")","0"},

  {"$","%","*","+","1"},
  {"A","B","C","","2"},
  {"D","E","F","","3"},
  {"G","H","I","","4"},
  {"J","K","L","","5"},
  {"M","N","O","","6"},
  {"P","Q","R","S","7"},
  {"T","U","V","","8"},
  {"W","X","Y","Z","9"},
  {",",".",":",";","0"}
};

//各種パラメータ
const int longpushborder = 10; //キー長押しボーダー
const int clickmargin = 50;
const int directionborder = 300; // フリック入力のマウス側の入力受付ボーダー
const int mousewheelsense = 80; //マウスホイール感度
int mouse_mode = 0;
int mouse_param_mode = 0;

//入力レジスタ
int mouseinput[2] = {0,0};
int keyinput[4] = {0,0,0,0};
int keynumber = 0;
int keyenable = 0;


//出力レジスタ
int mouseoutput[2] = {0,0};
boolean mousebtnoutput[3] = {false,false,false};
String keyoutput = "";
int funckey = 0;
int sounds[3] = {0,0,0};

//内部フラグなど
  //入力モード(0:かなモード 1:英語モード)
  int key_mode = 0;
  // 直前（１個前のメインループで押してた）キー入力
  int lastkey = -1;
  // 直前のdirectionの値
  int lastdirection = 0;
  // 小文字入力を行っていいか？
  int pushedkeyflag = 0;
  // 前回確定したキー入力
  int pushedkey[3] = {-1,-1,0};
  //フリック入力の方向保持用
  int direction = 0;
  //フリック入力が前回より継続されているかどうか
  int nowflicking = 0;
  //キー入力が前回より継続されているか　/ されていないか
  int pushflag = 0;
  //マウスホイール計算用
  float mousewheelcounter = 0;
  //マウスクリックの継続時間
  int mouseclickcounter = 0;
  // shiftキー
  int shift = 0;
//内部フラグおわり


//いい感じのイージング関数
float easing(float input){
   if(input < 0){
      return 0.0;
   }
   if(input > 1){
      return 1.0;
   }

   if(input > 0.5){
      return 4 * pow(input,3);
   } else {
      return 1 - (pow(2-2*input,3)/2);
   }
}

//速度をマウス出力にする比較的簡易なコード
void mouse_calc3(int mousex, int mousey){
  int i, minusflag;
  float mouseinput[2];
  mouseinput[0] = float(mousex) - 552;
  mouseinput[1] = float(mousey) - 532;
  float mousedeltas[2];
  static int lastmouseinput[2] = {512,512};
   for(i=0; i<2; i++){
    mousedeltas[i] = float(lastmouseinput[i]) - mouseinput[i];
    float theta = mousedeltas[0]*(mouseinput[0]) + mousedeltas[1]*(mouseinput[1]);
    theta /= sqrt((mousedeltas[0]*mousedeltas[0] + mousedeltas[1]*mousedeltas[1]))*sqrt((mouseinput[0])*(mouseinput[0]) + (mouseinput[1])*(mouseinput[1]));
    if(theta > 0) {
      mousedeltas[i]/=3;
    } 
    if(mousedeltas[i] < 0){
      minusflag = 1;
    }else{
      minusflag = 0;
    }
    mousedeltas[i] = pow(abs(mousedeltas[i]), 1.5);
    if(minusflag==1) mousedeltas[i] *= -1;
    mousedeltas[i] *= 0.2;
    mouseoutput[i] = -1 * mousedeltas[i];
   }
   lastmouseinput[0] = mousex - 552;
   lastmouseinput[1] = mousey - 532;
}

//入力値をそのままマウス出力にするだけのシンプルなコード(デットゾーン設定済み)
void mouse_calc2(int mousex, int mousey){
  int mouseinput[2] = {0,0};

  mouseinput[0] = float(mousex) - 552;
  mouseinput[1] = float(mousey) - 532;
  int i;
  for(i=0;i<2;i++){
    if(abs(mouseinput[i]) < 60){
      mouseinput[i] = 0;
    }

    mouseinput[i] /= 20;
    mouseoutput[i] = mouseinput[i];
  }

}
//抵抗値→マウスの移動量に変換する
void mouse_calc(int mousex, int mousey){
  const float params[9][4] = {
    {1.5, 0.7, 24, 2},
    {0.5, 0.7, 24, 2},
    {1,   0.7, 24, 2},
    {1.5, 0.7, 24, 2},
    {2,   0.7, 24, 2},
    {1.5, 0.5, 24, 2},
    {1.5, 1,   24, 0.4},
    {1.5, 1.5, 24, 0.08},
    {1.5, 2,   24, 0.02},
  };

  //2次曲線 + 操作時の速度モデル
  static float offset[2] = {512,512}; //初期位置
  static float deadzone = 300; //デットゾーン半径
  static float deadzone2 = 450;
  static float pow_exp = 1.5; //位置曲線の指数倍数
  static float delta_pow_exp = 0.7; //速度の指数倍数
  static float maxrate = 30;  //位置による最大マウス速度(x,y方向それぞれ)
  static float move_delta_factor = 1.4; //加速度の出力係数
  
  pow_exp = params[mouse_param_mode][0];
  delta_pow_exp = params[mouse_param_mode][1];
  maxrate = params[mouse_param_mode][2];
  move_delta_factor = params[mouse_param_mode][3];


  static int lastmouseinput[2] = {512,512};
  float mouseinput[2] = {0,0};
  float mousedeltas[2] = {0,0};
  float minusflag = 0;
  int i;
  
  mouseinput[0] = float(mousex);
  mouseinput[1] = float(mousey);
  float nativeinput[2] = {mousex-512, mousey-512};
  float inputnorm = sqrt(nativeinput[0]*nativeinput[0]+nativeinput[1]*nativeinput[1]);
  float theta = 0;

  //速度による影響度合いの決定
  for(i=0; i<2; i++){
    mousedeltas[i] = float(lastmouseinput[i]) - mouseinput[i];
    //中心から離れる動きのみ計算する（中心に戻る動きはカットする）
    //ゴムで初期位置に戻しても実際はいくらかズレるので40pxのデットゾーンを設定。
    theta = mousedeltas[0]*(mouseinput[0]-offset[0]) + mousedeltas[1]*(mouseinput[1]-offset[1]);
    theta /= sqrt((mousedeltas[0]*mousedeltas[0] + mousedeltas[1]*mousedeltas[1]))*sqrt((mouseinput[0]-offset[0])*(mouseinput[0]-offset[0]) + (mouseinput[1]-offset[1])*(mouseinput[1]-offset[1]));
    if(theta > 0 || inputnorm < 40) {
      mousedeltas[i]=0;
    } 
    if(mousedeltas[i] < 0){
      minusflag = 1;
    }else{
      minusflag = 0;
    }
  }

    float temp_norm = pow((mousedeltas[0]*mousedeltas[0]+mousedeltas[1]*mousedeltas[1]),0.5);
    float compute_norm = pow(temp_norm,delta_pow_exp);

  for(i=0; i<2; i++){
    mousedeltas[i] = mousedeltas[i] * (compute_norm / temp_norm);
    if(minusflag==1) mousedeltas[i] *= -1;
    mousedeltas[i] *= move_delta_factor;

  }
  
  //現在の座標による移動量の決定
  for(i=0; i<2; i++){
    mouseinput[i] -= offset[i];
    //デットゾーン内部なら0にして、それ以外ではデットゾーン分を引いたベクトルを求める
    if(inputnorm < float(deadzone)) {
      mouseinput[i] = 0;
    } else {
      float normratio = 1- (deadzone/inputnorm);
      mouseinput[i] = (mouseinput[i] * normratio);
    }
    if(mouseinput[i]<0){
      minusflag = 1;
    }else{
      minusflag = 0;
    }
    mouseinput[i] /= (512 - deadzone);
}

    temp_norm = pow((mouseinput[0]*mouseinput[0]+mouseinput[1]*mouseinput[1]),0.5);
    compute_norm = pow(temp_norm,pow_exp);

for(i=0; i<2; i++){
    mouseinput[i] = mouseinput[i] * (compute_norm / temp_norm);
    mouseinput[i] *= maxrate;
    if(minusflag==1) mouseinput[i] *= -1;
    //素早く中心に戻そうとする動きの時は移動を0にする
    if(theta > 0 && sqrt(mousedeltas[0]*mousedeltas[0]+mousedeltas[1]*mousedeltas[1]) > 5) {
      mouseinput[i] = 0;
    }
  }
  

  //イージング関数で合算する
  if(inputnorm > deadzone && inputnorm < deadzone2){
      float q = (inputnorm - deadzone) / (deadzone2 - deadzone);
      float eq = easing(q);
      mouseoutput[i] = int((1-eq)*-1*mousedeltas[i]) + int(eq * mouseinput[i]);
  } else if(inputnorm <= deadzone) {
    for(i=0; i<2; i++){
      mouseoutput[i] = int(-1*mousedeltas[i]);
    }
  } else {
    for(i=0; i<2; i++){
      mouseoutput[i] = int(mouseinput[i]);
    }
  }

  /*
    float delta_norm = sqrt(mousedeltas[0]*mousedeltas[0] + mousedeltas[1]*mousedeltas[1]);
    if(sumvec_norm > maxrate*1.5 && delta_norm < 3){
      for(i=0; i<2; i++){
        mouseoutput[i] = int(-1*mousedeltas[i]);
      }
    } else {
      for(i=0; i<2; i++){
        mouseinput[i] -= mousedeltas[i];
        mouseoutput[i] = int(mouseinput[i]);
      }
    }
  */

  lastmouseinput[0] = mousex;
  lastmouseinput[1] = mousey;
}
//プルダウン回路用の信号反転関数
int signalnot(int input){
  if(input==0){
    return 1;
  } else {
    return 0;
  }
}
//サウンドを再生する
void soundset(int i, int j, int k){
  sounds[0] = i;
  sounds[1] = j;
  sounds[2] = k;
}


//入力処理
void input(){
  mouseinput[0] = analogRead(MOUSEPIN_X);
  mouseinput[1] = analogRead(MOUSEPIN_Y);

  keyinput[0] = digitalRead(KBD_1);
  keyinput[1] = digitalRead(KBD_2);
  keyinput[2] = digitalRead(KBD_3);
  keyinput[3] = digitalRead(KBD_4);
  keynumber = keyinput[3]*8 + keyinput[2]*4 + keyinput[1]*2 + keyinput[0];
  keyenable = digitalRead(KBD_E);

  mousebtnoutput[0]= signalnot(digitalRead(16));
  mousebtnoutput[1]= signalnot(digitalRead(10));

  if(digitalRead(7) == 0){
    mouse_mode++;
    mouse_mode %= 3;
    Serial.print("マウス計算式変更:");
    Serial.println(mouse_mode);
    tone(9,6000);
    delay(10);
    while(digitalRead(7) == 0){};
    noTone(9);
  }

  if(digitalRead(8) == 0){
    mouse_param_mode++;
    mouse_param_mode %= 9;
    Serial.print("マウスパラメータ変更:");
    Serial.println(mouse_param_mode);
    tone(9,6000);
    delay(10);
    while(digitalRead(8) == 0){};
    noTone(9);
  }

}
//かな/英の入力モード切り替え&初期化処理
void ModeChange(){
    Keyboard.press(KEYCODE_CTRL);
    delay(100);
    Keyboard.press(KEYCODE_SPACE);
    delay(10);
    Keyboard.releaseAll();
    if(key_mode){
      key_mode = 0;
      tone(SOUND_PIN, 1000);
      delay(100);
      tone(SOUND_PIN, 2000);
      delay(100);
      tone(SOUND_PIN, 3000);
      delay(100);
      noTone(SOUND_PIN);
    } else {
      key_mode = 1;
      tone(SOUND_PIN, 3000);
      delay(100);
      tone(SOUND_PIN, 2000);
      delay(100);
      tone(SOUND_PIN, 1000);
      delay(100);
      noTone(SOUND_PIN);
    }
    lastdirection = 0;
    pushedkeyflag = 0;
    pushedkey[0]= -1;
    pushedkey[1]= -1;
    pushedkey[2]= 0;
    direction = 0;
    nowflicking = 0;
    pushflag = 0;
    mousewheelcounter = 0;
    mouseclickcounter = 0;
}

void compute(){
  //マウスホイールが動かされる処理を最優先
  if(keyenable==1 && (keynumber == MOUSEWHEEL_PIN )){
      pushedkeyflag = 0;
      mouse_calc(float(mouseinput[0]),float(mouseinput[1]));
      mousebtnoutput[2] = true;
  //なにかのキーが押されてるとき
  } else if( keyenable == 1){
     //キー入力中はマウス入力に応じてdirectionの値にどの方向にフリックしたか保持する
      if(abs(mouseinput[0] - 512) > directionborder || abs(mouseinput[1] - 512) > directionborder){
        nowflicking ++;
        if(nowflicking > 10000) nowflicking=100;
        if(abs(mouseinput[0] - 512) > abs(mouseinput[1] - 512)){
          if(mouseinput[0] - 512 > 0 ){
            direction = 3;
          } else {
            direction = 1;
          }
        } else {
          if(mouseinput[1] - 512 > 0 ){
            direction = 4;
          } else {
            direction = 2;
          }
        }
      } else {
        nowflicking = 0;
      }

      //キーが押されている間の各キーの処理
      switch(keynumber){
        case ENTER_KEY_PIN:
          if(lastkey != ENTER_KEY_PIN){
            soundset(1,800,3);
          }
          if(nowflicking == 1){
            if(direction == 4){
              funckey = KEYCODE_ENTER;
              soundset(1,4000,3);
            } else if(direction == 1){
              funckey = KEYCODE_BACKSPACE;
              soundset(1,400,3);
            } else if(direction == 3){
              funckey = KEYCODE_SPACE;
              soundset(1,1300,3);
            }
          } if(nowflicking > 50 && nowflicking%5==0){
            if(direction == 1){
              funckey = KEYCODE_BACKSPACE;
              soundset(1,400,3);
            }
          }
          break;

        case FUNC_KEY_PIN:
          if(key_mode==0){ 
            if(pushedkeyflag == 1 && pushflag==0){
              if(pushedkey[1] == 3 && pushedkey[2]==2){
                pushedkey[1] = 10;
              }
              if(pushedkey[1] == 7 && (pushedkey[2]==0 || pushedkey[2]==2 || pushedkey[2]==4)){
                pushedkey[1] = 11;
              }
              pushedkey[0]++;
              pushedkey[0] %= keymaplength[pushedkey[1]];
              //押した瞬間にキー出力
              keyoutput = kana_keymap[pushedkey[0]][pushedkey[1]];
              keyoutput += siin[keymapsiinlist[pushedkey[1]]][pushedkey[2]];
              soundset(1,3000,2);
              funckey = KEYCODE_BACKSPACE;
            }
          } else {
             if(lastkey == -1){
                if(shift==0){
                  shift=1;
                }else{
                  shift=0;
                }
                soundset(1,600,10);
              }
          }
          break;

        case ARROW_KEY_PIN:
          if(key_mode == 1){
            if(nowflicking == 1){
              if(direction == 1){
                funckey = KEYCODE_ARROW_LEFT;
                soundset(1,4000,3);
              } else if(direction == 2){
                funckey = KEYCODE_ARROW_UP;
                soundset(1,400,3);
              } else if(direction == 3){
                funckey = KEYCODE_ARROW_RIGHT;
                soundset(1,1300,3);
              } else if(direction == 4){
                funckey = KEYCODE_ARROW_DOWN;
                soundset(1,1300,3);
              }
            }
          } else {
            mouse_calc(float(mouseinput[0]),float(mouseinput[1]));

          }
        break;

        case MODE_CHANGE_PIN:
          if(lastkey == -1){
            ModeChange();
          }
        break;

        default:
          pushedkeyflag = 1;
          pushedkey[0] = 0;
          pushedkey[1] = keynumber;
          if(pushedkey[1] == 10) pushedkey[1] = 9;
          if(key_mode == 0){
            //押した瞬間にとりあえずキー出力
            if(lastkey == -1){
              keyoutput = kana_keymap[pushedkey[0]][pushedkey[1]];
              keyoutput += siin[keymapsiinlist[pushedkey[1]]][direction];
              soundset(1,2000,4);
            //フリック入力の向きが変わったらその都度表示を更新
            } else if(lastdirection != direction){
              keyoutput = kana_keymap[pushedkey[0]][pushedkey[1]];
              keyoutput += siin[keymapsiinlist[pushedkey[1]]][direction];
              funckey = KEYCODE_BACKSPACE;
              soundset(1,2500,1);
            }
          } else {
            if(lastkey == -1){
              keyoutput = alp_keymap[pushedkey[1] + (shift*10)][direction];
              soundset(1,2000,4);
            } else if(lastdirection != direction){
              keyoutput = alp_keymap[pushedkey[1] + (shift*10)][direction];
              funckey = KEYCODE_BACKSPACE;
              soundset(1,2500,1);
            }
          }
          break;
      }
      pushflag = 1;

  //キーが押されていないとき
  } else {
      pushflag=0;
      //そうでなければマウスモードになる
      switch(lastkey){
        case -1:
          //マウス座標計算
          switch(mouse_mode){
            case 0:
              mouse_calc(float(mouseinput[0]),float(mouseinput[1]));
              break;
            case 1:
              mouse_calc2(float(mouseinput[0]),float(mouseinput[1]));
              break;
            case 2:
            default:
              mouse_calc3(float(mouseinput[0]),float(mouseinput[1]));
              break;
          }
          //マウスクリック中一定時間はポインタを動かさない
            if(mousebtnoutput[0] || mousebtnoutput[1]){
              mouseclickcounter++;
              if(mouseclickcounter < clickmargin){
                mouseoutput[0] = 0;
                mouseoutput[1] = 0;
              }
            } else{
              mouseclickcounter = 0;
            }
          break;

        case ENTER_KEY_PIN:
          if(direction == 0){
            funckey = KEYCODE_SPACE;
          }
          pushedkeyflag = 0;
          lastkey = -1;
          pushedkey[2] = direction;
          direction = 0;
          break;

        case FUNC_KEY_PIN:
          lastkey = -1;
          direction = 0;
          if(key_mode==1) pushedkey[2] = direction;
          break;

        case MODE_CHANGE_PIN:
        case ARROW_KEY_PIN:
        break;

        default:
          lastkey = -1;
          pushedkey[2] = direction;
          direction = 0;
          break;
      }
  }
  if(keyenable == 1) {
    lastkey = keynumber;
  } else {
    lastkey = -1;
  }
  lastdirection = direction;
}


void output(){
    if(mousebtnoutput[0]==true){
      Mouse.press(MOUSE_LEFT);
    } else {
      Mouse.release(MOUSE_LEFT);
    }
    if(mousebtnoutput[1]==true){
      Mouse.press(MOUSE_RIGHT);
    } else {
      Mouse.release(MOUSE_RIGHT);
    }
    
    if(funckey != 0){
      Keyboard.press(funckey);
      delay(10);
      Keyboard.release(funckey);
    }
    if(keyoutput != ""){
      Keyboard.print(keyoutput);
    }


    if(mousebtnoutput[2] == false){
      Mouse.move(mouseoutput[0],mouseoutput[1]);
    } else {
      mousewheelcounter += mouseoutput[1];
      if(abs(mousewheelcounter) > mousewheelsense){
        Mouse.move(0,0,int(-1 * mousewheelcounter/mousewheelsense));
        if(mousewheelcounter > 0){
          mousewheelcounter -= mousewheelsense;
        } else {
          mousewheelcounter += mousewheelsense;
        }
      } 
    }

    if(sounds[0] == 1){
      sounds[0] = 0;
      tone(9,sounds[1]);
    }
    if(sounds[2] > 0){
      sounds[2]--;
    } else {
      noTone(9);
      sounds[2]=0;
    }

    mouseoutput[0] = 0;
    mouseoutput[0] = 0;
    mousebtnoutput[0] = false;
    mousebtnoutput[1] = false;
    mousebtnoutput[2] = false;
    funckey = 0;
    keyoutput = "";
}

void setup()
{
  Serial.begin(9600 * 2);
  Mouse.begin();
  Keyboard.begin();
  pinMode( 4, INPUT);
  pinMode( 5, INPUT);
  Serial.print("test ");
}

void loop() {
  input();
  compute();
  output();
  delay(10);
}
