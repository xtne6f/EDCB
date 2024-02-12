// Only ASCII characters can be used here.

namespace EpgTimer
{
    public partial class App : System.Windows.Application
    {
        internal const string VERSION_TEXT = (VERSION_TAG != "" ? " " + VERSION_TAG : "") + (VERSION_EXTRA != "" ? " " + VERSION_EXTRA : "");
        const string VERSION_TAG = "work+s-240212";
        const string VERSION_EXTRA = "";
    }
}
