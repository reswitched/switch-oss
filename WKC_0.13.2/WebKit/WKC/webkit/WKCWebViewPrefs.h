/*
 * WKCWebViewPrefs.h
 *
 * Copyright (c) 2011, 2012 ACCESS CO., LTD. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef WKCWebViewPrefs_h
#define WKCWebViewPrefs_h

#include <WKCWebView.h>

namespace WKC {

/*@{*/

/** @brief Class that sets preferences for each View. */
class WKC_API WKCWebViewPrefs
{
public:
    /**
       @brief Generates WKCWebViewPrefs
       @param view Pointer to WKCWebView
       @retval !=0 Pointer to WKCWebViewPrefs
       @retval ==0 Failed to generate
    */
    static WKCWebViewPrefs* create(WKCWebView* view);
    /**
       @brief Discards WKCWebViewPrefs
       @param self Pointer to WKCWebView
       @retval None
    */
    static void deleteWKCWebViewPrefs(WKCWebViewPrefs *self);
    /**
       @brief Forcibly terminates WKCWebViewPrefs
       @retval None
    */
    static void forceTerminate();
    /**
       @brief Initializes WKCWebViewPrefs Global variable
       @retval None
    */
    static void resetVariables();

    /**
       @brief Sets whether to enable JavaScript:
       @param flag JavaScript: activation setting value
       - != false Enable @n
       - == false Do not enable @n
       @return None
    */
    void setJavaScriptURLsAreAllowed(bool flag);
    /**
       @brief Checks if transparency is specified
       @retval "!= false" Transparency is enabled
       @retval "== false" Transparency is disabled 
    */
    bool transparent();
    /**
       @brief Sets whether transparency is specified
       @param flag Transparency setting value 
       - != false Transparency is enabled 
       - == false Transparency is disabled 
       @return None
       @details
       When transparency setting is enabled, filling of the background on the offscreen will be performed only for content that specifies a background in the body.@n
       To display transparency, the target rectangle must be cleared before using WKC::WKCWebView::notifyPaintOffscreen().@n
    */
    void setTransparent(bool flag);

private:
    WKCWebViewPrefs(WKCWebView* view);
    ~WKCWebViewPrefs();
    bool construct();

private:
    WKCWebView* m_view;
    WKCWebViewPrivate* m_privateView;
};

/*@}*/

} // namespace
#endif // WKCWebViewPrefs_h
