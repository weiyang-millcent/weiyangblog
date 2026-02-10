// scripts/fix-giscus.js
// 修复Butterfly主题自带的Giscus评论系统

hexo.extend.filter.register('after_render:html', function(str, data) {
  // 在页面HTML渲染完成后修复Giscus
  if (data.path && data.path.includes('post')) {
    // 替换可能出问题的懒加载配置
    str = str.replace(/data-loading="lazy"/g, 'data-loading="eager"');
    
    // 确保Giscus主题设置正确
    str = str.replace(/data-theme="preferred_color_scheme"/g, 'data-theme="light" data-loading="eager"');
    
    // 添加调试信息
    console.log('✅ 已修复Butterfly主题Giscus配置');
  }
  return str;
});

// 添加JavaScript修复
hexo.extend.injector.register('body_end', `
<script>
// 修复Giscus加载问题
document.addEventListener('DOMContentLoaded', function() {
  console.log('🔧 开始修复Giscus...');
  
  // 等待2秒确保主题JS执行完毕
  setTimeout(function() {
    const commentContainer = document.getElementById('post-comment');
    const giscusScript = document.querySelector('script[src*="giscus"]');
    
    if (commentContainer && !commentContainer.querySelector('.giscus-frame')) {
      console.log('⚠️ Giscus未显示，尝试修复...');
      
      // 方法1：强制重新加载Giscus
      if (giscusScript) {
        console.log('重新加载Giscus脚本...');
        const newScript = giscusScript.cloneNode(true);
        giscusScript.remove();
        document.body.appendChild(newScript);
      }
      
      // 方法2：如果还是没有，显示备用评论区域
      setTimeout(function() {
        if (!document.querySelector('.giscus-frame')) {
          console.log('显示备用评论区域...');
          const fallbackHTML = \`
            <div style="padding: 20px; background: #f8f9fa; border-radius: 8px; margin-top: 20px;">
              <h4>💬 评论</h4>
              <p>如果评论框未显示，请 <a href="#" onclick="location.reload()">刷新页面</a> 或直接访问 
              <a href="https://github.com/weiyang-millcent/weiyangblog/discussions" target="_blank">GitHub Discussions</a></p>
            </div>
          \`;
          commentContainer.insertAdjacentHTML('beforeend', fallbackHTML);
        }
      }, 5000);
    } else {
      console.log('✅ Giscus正常显示');
    }
  }, 2000);
});
</script>
`, 'default');
