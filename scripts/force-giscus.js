// scripts/force-giscus.js
// 强制注入Giscus评论系统，绕过Butterfly主题的bug

hexo.extend.filter.register('after_post_render', function(data) {
  // 只在文章页面注入，且确保文章允许评论
  if (data.layout === 'post' && data.comments !== false) {
    
    // Giscus HTML代码
    const giscusHTML = `
<!-- 强制注入的Giscus评论系统 -->
<div class="giscus-comments-section">
  <script src="https://giscus.app/client.js"
          data-repo="weiyang-millcent/weiyangblog"
          data-repo-id="R_kgDORKpzYA"
          data-category="Announcements"
          data-category-id="DIC_kwDORKpzYM4C2HoZ"
          data-mapping="pathname"
          data-strict="0"
          data-reactions-enabled="1"
          data-emit-metadata="0"
          data-input-position="bottom"
          data-theme="preferred_color_scheme"
          data-lang="zh-CN"
          crossorigin="anonymous"
          async>
  </script>
</div>
`;
    
    // 在文章内容后直接追加
    data.content += giscusHTML;
    
    // 添加日志
    console.log(`✅ 已为文章 "${data.title}" 注入Giscus评论系统`);
  }
  return data;
});

console.log('🚀 Giscus强制注入脚本已加载');
