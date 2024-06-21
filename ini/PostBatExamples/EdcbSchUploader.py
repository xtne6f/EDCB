# EDCBの予約をしょぼいカレンダーにアップロードするPythonのスクリプト
# ・スクリプトと同じフォルダに https://github.com/xtne6f/edcb.py が必要
# ・仕様は基本的にtvrockSchUploader.jsなどと同じ
# ・以前の送信内容を.lastファイルに書き込み、変化がなければ次回の送信は行わない
# ・PowerShell版(EdcbSchUploader.ps1)と内容は同じ

import asyncio
import base64
import datetime
import os
import sys
import urllib.parse
import urllib.request
sys.dont_write_bytecode = True
import edcb

# 使用予定チューナーの配色
DEV_COLORS = 'T9999=#ff0000\t#1e90ff'
# ユーザーエージェント
USER_AGENT = 'edcbSchUploader/1.0.0'
# アップロード先
UPLOAD_URL = 'https://cal.syoboi.jp/sch_upload'
# 2024年時点ではHTTPも使える模様 (HTTPSでのアップロードが不調な場合などに)
#UPLOAD_URL = 'http://cal.syoboi.jp/sch_upload'
# 通信タイムアウト(ミリ秒)
TIMEOUT = 10000
# 番組名を省略する文字数
UPLOAD_TITLE_MAXLEN = 30
# 録画優先度がこれ以上の予約だけ送信する
UPLOAD_REC_PRIORITY = 1

async def main():
    if len(sys.argv) < 3:
        print('Usage: EdcbSchUploader <user> <pass> [epgurl] [slot]')
        sys.exit(2)

    # サービス名としょぼかる放送局名の対応ファイル"SyoboiCh.txt"があれば読み込む
    ch_map = {}
    ch_txt_path = os.path.join(os.path.dirname(__file__), 'SyoboiCh.txt')
    if os.path.isfile(ch_txt_path):
        with open(ch_txt_path, 'rb') as f:
            for line in edcb.EDCBUtil.convertBytesToString(f.read(), 'cp932' if sys.platform == 'win32' else 'utf_8').splitlines():
                a = line.split('\t')
                if len(a) >= 2 and not a[0].startswith(';'):
                    ch_map[a[0]] = a[1]

    # EpgTimerSrvから予約情報を取得
    cmd = edcb.CtrlCmdUtil()
    rl = await cmd.sendEnumReserve()
    tl = await cmd.sendEnumTunerReserve()
    if rl is None or tl is None:
        print('Failed to communicate with EpgTimerSrv')
        sys.exit(1)

    data = []
    for r in rl:
        tuner = None
        for tr in tl:
            if r['reserve_id'] in tr['reserve_list']:
                # T+{BonDriver番号(上2桁)}+{チューナ番号(下2桁)},チューナ不足はT9999
                tuner_id = min(tr['tuner_id'] // 65536, 99) * 100 + min(tr['tuner_id'] % 65536, 99)
                tuner = f'T{tuner_id:04}'
                break

        if tuner is not None and r['rec_setting']['rec_mode'] <= 3 and r['rec_setting']['priority'] >= UPLOAD_REC_PRIORITY:
            # "最大200行"
            if len(data) >= 200:
                break

            # "開始時間 終了時間 デバイス名 番組名 放送局名 サブタイトル オフセット XID"
            title = r['title'].replace('\t', '').replace('\r', '').replace('\n', '')
            if len(title) > UPLOAD_TITLE_MAXLEN:
                title = title[0 : UPLOAD_TITLE_MAXLEN - 1] + '…'
            station_name = r['station_name']
            if station_name in ch_map:
                station_name = ch_map[station_name]

            # サロゲートペアを送ると500エラーが返るため
            title = ''.join(filter(lambda c: ord(c) < 0x10000, title))
            station_name = ''.join(filter(lambda c: ord(c) < 0x10000, station_name))
            data.append(str(int(r['start_time'].timestamp())) + '\t' +
                        str(int((r['start_time'] + datetime.timedelta(seconds=r['duration_second'])).timestamp())) + '\t' +
                        tuner + '\t' + title + '\t' + station_name + '\t0\t' + str(r['reserve_id']))

    # 以前の内容と同じなら送信しない
    last_path = os.path.splitext(__file__)[0] + '.last'
    if os.path.isfile(last_path):
        with open(last_path, encoding='utf-8') as f:
            if f.read() == '\n'.join(data):
                print('Not modified, nothing to do.')
                return

    # 予約情報をサーバに送信
    headers = {
        'Authorization': 'Basic ' + base64.b64encode((sys.argv[1] + ':' + sys.argv[2]).encode()).decode(),
        'Content-Type': 'application/x-www-form-urlencoded',
        'User-Agent': USER_AGENT,
    }
    body = {
        'slot': sys.argv[4] if len(sys.argv) >= 5 else '0',
        'devcolors': DEV_COLORS,
        'epgurl': sys.argv[3] if len(sys.argv) >= 4 else '',
        'data': '\n'.join(data),
    }
    req = urllib.request.Request(UPLOAD_URL + '?slot=' + urllib.parse.quote(body['slot'], safe=''), urllib.parse.urlencode(body).encode(), headers)
    try:
        with urllib.request.urlopen(req, timeout=TIMEOUT / 1000) as res:
            pass
    except urllib.error.HTTPError as err:
        print(f'Response Error ({err.code})')
        sys.exit(1)
    except urllib.error.URLError as err:
        print(err.reason)
        sys.exit(1)

    with open(last_path, 'w', encoding='utf-8') as f:
        f.write(body['data'])

    print('Done.')

asyncio.run(main())
