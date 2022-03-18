/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/pattern.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"
#include "url/origin.h"

// Tests of the blob: URL scheme. Upstream implemented in
// content/browser/blob_storage/blob_url_browsertest.cc.
// Migrated from content_browsertests to brave_browser_tests.
class BlobUrlBrowserTest : public EphemeralStorageBrowserTest {
 public:
  void SetUpOnMainThread() override {
    EphemeralStorageBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->Start());
  }
};

IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest, LinkToUniqueOriginBlob) {
  // Use a data URL to obtain a test page in a unique origin. The page
  // contains a link to a "blob:null/SOME-GUID-STRING" URL.
  auto* rfh = ui_test_utils::NavigateToURL(
      browser(),
      GURL("data:text/html,<body><script>"
           "var link = document.body.appendChild(document.createElement('a'));"
           "link.innerText = 'Click Me!';"
           "link.href = URL.createObjectURL(new Blob(['potato']));"
           "link.target = '_blank';"
           "link.id = 'click_me';"
           "</script></body>"));
  ASSERT_TRUE(rfh);

  // Click the link.
  content::WebContentsAddedObserver window_observer;
  EXPECT_TRUE(ExecJs(rfh, "document.getElementById('click_me').click()"));
  content::WebContents* new_contents = window_observer.GetWebContents();
  EXPECT_TRUE(WaitForLoadStop(new_contents));

  EXPECT_TRUE(
      base::MatchPattern(new_contents->GetVisibleURL().spec(), "blob:null/*"));
  EXPECT_EQ(
      "null potato",
      EvalJs(new_contents, "self.origin + ' ' + document.body.innerText;"));
}

IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest, LinkToSameOriginBlob) {
  // Using an http page, click a link that opens a popup to a same-origin blob.
  GURL url = https_server_.GetURL("chromium.org", "/title1.html");
  url::Origin origin = url::Origin::Create(url);
  auto* rfh = ui_test_utils::NavigateToURL(browser(), url);

  content::WebContentsAddedObserver window_observer;
  EXPECT_TRUE(ExecJs(
      rfh,
      "var link = document.body.appendChild(document.createElement('a'));"
      "link.innerText = 'Click Me!';"
      "link.href = URL.createObjectURL(new Blob(['potato']));"
      "link.target = '_blank';"
      "link.click()"));

  // The link should create a new tab.
  content::WebContents* new_contents = window_observer.GetWebContents();
  EXPECT_TRUE(WaitForLoadStop(new_contents));

  EXPECT_TRUE(base::MatchPattern(new_contents->GetVisibleURL().spec(),
                                 "blob:" + origin.Serialize() + "/*"));
  EXPECT_EQ(
      origin.Serialize() + " potato",
      EvalJs(new_contents, "    self.origin + ' ' + document.body.innerText;"));
}

// Regression test for https://crbug.com/646278
IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest, LinkToSameOriginBlobWithAuthority) {
  // Using an http page, click a link that opens a popup to a same-origin blob
  // that has a spoofy authority section applied. This should be blocked.
  GURL url = embedded_test_server()->GetURL("chromium.org", "/title1.html");
  url::Origin origin = url::Origin::Create(url);
  auto* rfh = ui_test_utils::NavigateToURL(browser(), url);

  content::WebContentsAddedObserver window_observer;
  EXPECT_TRUE(ExecJs(
      rfh,
      "var link = document.body.appendChild(document.createElement('a'));"
      "link.innerText = 'Click Me!';"
      "link.href = 'blob:http://spoof.com@' + "
      "    URL.createObjectURL(new Blob(['potato'])).split('://')[1];"
      "link.rel = 'opener'; link.target = '_blank';"
      "link.click()"));

  // The link should create a new tab.
  content::WebContents* new_contents = window_observer.GetWebContents();
  EXPECT_TRUE(WaitForLoadStop(new_contents));

  // The spoofy URL should not be shown to the user.
  EXPECT_FALSE(
      base::MatchPattern(new_contents->GetVisibleURL().spec(), "*spoof*"));
  // The currently implemented behavior is that the URL gets rewritten to
  // about:blank#blocked.
  EXPECT_EQ(content::kBlockedURL, new_contents->GetVisibleURL().spec());
  EXPECT_EQ(
      origin.Serialize() + " ",
      EvalJs(new_contents,
             "self.origin + ' ' + document.body.innerText;"));  // no potato
}

// Regression test for https://crbug.com/646278
IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest, ReplaceStateToAddAuthorityToBlob) {
  // history.replaceState from a validly loaded blob URL shouldn't allow adding
  // an authority to the inner URL, which would be spoofy.
  GURL url = embedded_test_server()->GetURL("chromium.org", "/title1.html");
  url::Origin origin = url::Origin::Create(url);
  auto* rfh = ui_test_utils::NavigateToURL(browser(), url);

  content::WebContentsAddedObserver window_observer;
  EXPECT_TRUE(ExecJs(
      rfh,
      "var spoof_fn = function () {\n"
      "  host_port = self.origin.split('://')[1];\n"
      "  spoof_url = 'blob:http://spoof.com@' + host_port + '/abcd';\n"
      "  window.history.replaceState({}, '', spoof_url);\n"
      "};\n"
      "args = ['<body>potato<scr', 'ipt>(', spoof_fn, ')();</scri', 'pt>'];\n"
      "b = new Blob(args, {type: 'text/html'});"
      "window.open(URL.createObjectURL(b));"));

  content::WebContents* new_contents = window_observer.GetWebContents();
  EXPECT_TRUE(WaitForLoadStop(new_contents));

  // The spoofy URL should not be shown to the user.
  EXPECT_FALSE(
      base::MatchPattern(new_contents->GetVisibleURL().spec(), "*spoof*"));

  // The currently implemented behavior is that the URL gets rewritten to
  // about:blank#blocked. The content of the page stays the same.
  EXPECT_EQ(content::kBlockedURL, new_contents->GetVisibleURL().spec());
  EXPECT_EQ(
      origin.Serialize() + " potato",
      EvalJs(new_contents, "self.origin + ' ' + document.body.innerText;"));

  // TODO(nick): Currently, window.location still reflects the spoof URL.
  // This seems unfortunate -- can we fix it?
  std::string window_location =
      EvalJs(new_contents, "window.location.href;").ExtractString();
  EXPECT_TRUE(base::MatchPattern(window_location, "*spoof*"));
}

IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest, BlobsArePartitioned) {
  constexpr char kFetchBlobScript[] = R"(
  (async function() {
    const response = await fetch($1);
    return await response.text();
  })();)";

  std::vector<GURL> a_com_registered_blobs;
  {
    auto* a_com_main_frame =
        LoadURLInNewTab(a_site_ephemeral_storage_url_)->GetMainFrame();
    std::vector<content::RenderFrameHost*> a_com_rfhs{{
        a_com_main_frame,
        content::ChildFrameAt(a_com_main_frame, 0),
        content::ChildFrameAt(a_com_main_frame, 1),
        content::ChildFrameAt(a_com_main_frame, 2),
        content::ChildFrameAt(a_com_main_frame, 3),
    }};

    {
      int idx = 0;
      for (auto* rfh : a_com_rfhs) {
        a_com_registered_blobs.push_back(
            GURL(EvalJs(rfh, content::JsReplace(
                                 "URL.createObjectURL(new Blob(['$1']));", idx))
                     .ExtractString()));
        ASSERT_EQ(
            EvalJs(rfh, content::JsReplace(kFetchBlobScript,
                                           a_com_registered_blobs[idx].spec()))
                .ExtractString(),
            std::to_string(idx));
        ++idx;
      }
    }
  }

  std::vector<GURL> b_com_registered_blobs;
  {
    auto* b_com_main_frame =
        LoadURLInNewTab(b_site_ephemeral_storage_url_)->GetMainFrame();
    std::vector<content::RenderFrameHost*> b_com_rfhs{{
        b_com_main_frame,
        content::ChildFrameAt(b_com_main_frame, 0),
        content::ChildFrameAt(b_com_main_frame, 1),
        content::ChildFrameAt(b_com_main_frame, 2),
        content::ChildFrameAt(b_com_main_frame, 3),
    }};

    {
      int idx = 0;
      for (auto* rfh : b_com_rfhs) {
        b_com_registered_blobs.push_back(
            GURL(EvalJs(rfh, content::JsReplace(
                                 "URL.createObjectURL(new Blob(['$1']));", idx))
                     .ExtractString()));
        ASSERT_EQ(
            EvalJs(rfh, content::JsReplace(kFetchBlobScript,
                                           b_com_registered_blobs[idx].spec()))
                .ExtractString(),
            std::to_string(idx));

        // No blobs from a.com should be available.
        EXPECT_FALSE(
            EvalJs(rfh, content::JsReplace(kFetchBlobScript,
                                           a_com_registered_blobs[idx].spec()))
                .error.empty());
        ++idx;
      }
    }
  }

  // Expect all a.com blobs (including the ones from 3p frames) are available in
  // another a.com tab.
  {
    auto* a_com_main_frame =
        LoadURLInNewTab(a_site_ephemeral_storage_url_)->GetMainFrame();
    std::vector<content::RenderFrameHost*> a_com_rfhs{{
        a_com_main_frame,
        content::ChildFrameAt(a_com_main_frame, 0),
        content::ChildFrameAt(a_com_main_frame, 1),
        content::ChildFrameAt(a_com_main_frame, 2),
        content::ChildFrameAt(a_com_main_frame, 3),
    }};

    int idx = 0;
    for (auto* rfh : a_com_rfhs) {
      EXPECT_EQ(
          EvalJs(rfh, content::JsReplace(kFetchBlobScript,
                                         a_com_registered_blobs[idx].spec()))
              .ExtractString(),
          std::to_string(idx));
      ++idx;
    }
  }
}
