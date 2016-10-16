using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    public class MenuBinds
    {
        private static MenuManager mm { get { return CommonManager.Instance.MM; } }

        private List<ICommand> AppendGestureCmds = new List<ICommand>();
//        private List<ICommand> AppendWorkCmds = new List<ICommand>();
        public CtxmCode View { get; set; }

        public MenuBinds() { View = CtxmCode.EtcWindow; }

        public void AddInputCommand(ICommand icmd)
        {
            if (icmd == null) return;

            if (AppendGestureCmds.Contains(icmd) == false)
            {
                AppendGestureCmds.Add(icmd);
            }

            /*//コマンドバインディング用の追加分のリスト
            if (AppendWorkCmds.Contains(icmd) == false)
            {
                AppendWorkCmds.Add(icmd);
            }*/
        }

        //public void DeleteInputCommand(ICommand icmd){}

        //public void SetCommandToButton(Button btn, ICommand icmd, int id = 0, object data = null, bool swapICmd = false)
        public void SetCommandToButton(Button btn, ICommand icmd, int id = 0, object data = null)
        {
            //追加コマンド控え
            AddInputCommand(icmd);

            //ボタンへコマンド登録とショートカット表示のツールチップセット。
            btn.Command = icmd;
            btn.CommandParameter = new EpgCmdParam(typeof(Button), View, id, data);
            btn.ToolTip = "";
            btn.ToolTipOpening += new ToolTipEventHandler(ICmdTooltip);
            ToolTipService.SetShowOnDisabled(btn, true);
        }

        public static void ICmdTooltip(Object sender, ToolTipEventArgs e)
        {
            if (sender is ICommandSource == false || sender is FrameworkElement == false) return;

            ICommand icmd = (sender as ICommandSource).Command ;
            CtxmCode code = ((sender as ICommandSource).CommandParameter as EpgCmdParam).Code;
            var obj = sender as FrameworkElement;
            obj.ToolTip = (mm.IsGestureDisableOnView(icmd, code) == true ? null : GetInputGestureText(icmd)) ?? "";
            if (obj.ToolTip as string == "")
            {
                e.Handled = true;
            }
        }

        //ショートカットの更新。
        //主要画面用
        public void DeleteInputBindings(UIElement iTrgView, UIElement iTrgList = null)
        {
            try
            {
                var deleteCmds = mm.GetViewMenuCmdList(View);
                deleteCmds.AddRange(AppendGestureCmds);
                deleteCmds.AddRange(mm.GetWorkGestureCmdList(View));//不要なはずだが安全のため一応
                deleteCmds = deleteCmds.Distinct().ToList();

                DeleteInputBindingsTo(iTrgView, deleteCmds);
                DeleteInputBindingsTo(iTrgList, deleteCmds);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        private void DeleteInputBindingsTo(UIElement iTrg, List<ICommand> delList)
        {
            if (iTrg == null) return;

            var delBinds = iTrg.InputBindings.OfType<InputBinding>().Where(bind => delList.Contains(bind.Command) == true).ToList();
            delBinds.ForEach(item => iTrg.InputBindings.Remove(item));
        }
        
        public void ResetInputBindings(UIElement iTrgView, UIElement iTrgList = null)
        {
            try
            {
                DeleteInputBindings(iTrgView, iTrgList);

                var gestureCmds = mm.GetWorkGestureCmdList(View);
                gestureCmds.AddRange(AppendGestureCmds);
                gestureCmds = gestureCmds.Distinct().ToList();

                AddInputBindgsTo(iTrgView, gestureCmds, MenuCmds.GestureTrg.ToView);
                AddInputBindgsTo(iTrgList, gestureCmds, MenuCmds.GestureTrg.ToList);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        private void AddInputBindgsTo(UIElement iTrg, List<ICommand> cmdList, MenuCmds.GestureTrg spc)
        {
            if (iTrg == null) return;

            cmdList.ForEach(icmd =>
            {
                MenuCmds.CmdData cmdData;
                if (mm.MC.WorkCmdOptions.TryGetValue(icmd, out cmdData))
                {
                    if (((cmdData.GesTrg & spc) == spc || cmdData.GesTrg == MenuCmds.GestureTrg.None) && mm.IsGestureDisableOnView(icmd, this.View) == false)
                    {
                        iTrg.InputBindings.AddRange(GetInputBinding(icmd));
                    }
                }
                else
                {
                    iTrg.InputBindings.AddRange(GetInputBinding(icmd));
                }
            });
        }

        private List<InputBinding> GetInputBinding(ICommand icmd, int id = 0, object data = null)
        {
            var bindlist = new List<InputBinding>();

            InputGestureCollection list = GetInputGestureList(icmd);
            foreach (var gesture in list.OfType<InputGesture>())
            {
                var bind = new InputBinding(icmd, gesture);
                bind.CommandParameter = new EpgCmdParam(bind.Gesture.GetType(), View, id, data);
                bindlist.Add(bind);
            }
            return bindlist;
        }

        //private static InputGestureCollection GetInputGestureList(ICommand icmd)
        public static InputGestureCollection GetInputGestureList(ICommand icmd)
        {
            if (icmd == null) return new InputGestureCollection();

            InputGestureCollection igc;
            if (mm.MC.WorkCmdOptions.ContainsKey(icmd) == true)
            {
                igc = mm.MC.WorkCmdOptions[icmd].Gestures;
            }
            else
            {
                igc = icmd is RoutedCommand ? (icmd as RoutedCommand).InputGestures : new InputGestureCollection();
            }
            return new InputGestureCollection(igc);
        }

        public static string GetInputGestureText(ICommand icmd)
        {
            return GetInputGestureText(GetInputGestureList(icmd));
        }

        public static string GetInputGestureText(InputGestureCollection gestures)
        {
            if (gestures == null) return null;

            List<KeyGesture> ShortCutKeys = gestures.OfType<KeyGesture>().ToList();

            string ret = "";
            ShortCutKeys.ForEach(item => ret += (ret.Count() != 0 ? ", " : "") + GetInputGestureText(item));
            return ret == "" ? null : ret;
        }

        public static string GetInputGestureText(KeyGesture gesture)
        {
            if (gesture == null) return null;
            string ret = gesture.GetDisplayStringForCulture(System.Globalization.CultureInfo.CurrentCulture);
            return ret.Replace("Return", "Enter").Replace("Up", "↑").Replace("Down", "↓");
        }

        /* 必要なものだけ登録しようかと思ったけど、やっぱ多少余計なものがあっても全部登録でいい気がしてきた。
        /// <summary>コマンドバインディング集から、必要なコマンドを登録する。</summary>
        public void SetCommandBindings(CommandBindingCollection cBindsSrc, UIElement cTrgView, UIElement cTrgMenu)
        {
            try
            {
                var workCmds = new List<ICommand>(mm.WorCtxmCmdList[Code]);
                SetCommandBindgsTo(cBindsSrc, cTrgMenu, workCmds);

                workCmds.AddRange(AppendWorkCmds);
                SetCommandBindgsTo(cBindsSrc, cTrgView, workCmds.Distinct().ToList());
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        private void SetCommandBindgsTo(CommandBindingCollection cBindsSrc, UIElement cTrg, List<ICommand> cmdList)
        {
            if (cBindsSrc == null || cTrg == null || cmdList == null) return;

            cBindsSrc.CommandBindings.Clear();
            cmdList.ForEach(icmd =>
            {
                //重複登録しない
                if (cTrg.CommandBindings.OfType<CommandBinding>().FirstOrDefault(cBind => cBind.Command == icmd) == null)
                {
                    var newBind = cBindsSrc.OfType<CommandBinding>().FirstOrDefault(cBind => cBind.Command == icmd);
                    if (newBind != null)
                    {
                        cTrg.CommandBindings.Add(newBind);
                    }
                    else
                    {
                        throw new ArgumentException("CommandBinding not found", "cBindsSrc");
                    }
                }
            });
        }*/
    }
}
