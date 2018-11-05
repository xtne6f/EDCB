-- 録画終了時にファイル名にドロップ数をつけるスクリプト
ri = edcb.GetRecFileInfoBasic(env.RecInfoID)
if ri and ri.recFilePath ~= '' then
  ren = ri.recFilePath:gsub('%.[^\\/]-$', '-D'..ri.drops..'S'..ri.scrambles..'%0')
  if edcb.os.rename(ri.recFilePath, ren) then
    edcb.os.rename(ri.recFilePath..'.program.txt', ren..'.program.txt')
    edcb.os.rename(ri.recFilePath..'.err', ren..'.err')
    edcb.ChgPathRecFileInfo(ri.id, ren)
  end
end
