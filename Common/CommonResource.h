#define EDCB_VERSION_TAG "work+s-241008"

// Only ASCII characters can be used here.

#ifndef EDCB_VERSION_TEXT
#if defined(EDCB_VERSION_TAG) && defined(EDCB_VERSION_EXTRA)
#define EDCB_VERSION_TEXT " " EDCB_VERSION_TAG " " EDCB_VERSION_EXTRA
#elif defined(EDCB_VERSION_TAG)
#define EDCB_VERSION_TEXT " " EDCB_VERSION_TAG
#elif defined(EDCB_VERSION_EXTRA)
#define EDCB_VERSION_TEXT " " EDCB_VERSION_EXTRA
#endif
#endif

// Uncomment this for legacy systems.
//#define EDCB_RC_DIALOG_NO_FONT

#ifndef EDCB_RC_DIALOG_FONT
#if defined(EDCB_RC_DIALOG_FONT_YUGOTHIC)
#define EDCB_RC_DIALOG_FONT "Yu Gothic UI"
#elif defined(EDCB_RC_DIALOG_FONT_MEIRYO)
#define EDCB_RC_DIALOG_FONT "Meiryo UI"
#elif !defined(EDCB_RC_DIALOG_NO_FONT)
#define EDCB_RC_DIALOG_FONT "Meiryo UI"
#endif
#endif
