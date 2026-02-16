const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Arduino LED Matrix Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background-color: #121212;
      color: #ffffff;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
      margin: 0;
    }
    h1 { margin-bottom: 20px; }
    .grid-container {
      display: grid;
      grid-template-columns: repeat(12, 30px);
      grid-template-rows: repeat(8, 30px);
      gap: 5px;
      margin-bottom: 20px;
    }
    .pixel {
      width: 30px;
      height: 30px;
      background-color: #333;
      border-radius: 5px;
      cursor: pointer;
      transition: background-color 0.1s;
    }
    .pixel.active {
      background-color: #ff0000;
      box-shadow: 0 0 10px #ff0000;
    }
    .controls-container {
      display: flex;
      flex-direction: column;
      gap: 15px;
      align-items: center;
      background: #1e1e1e;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    }
    .row {
      display: flex;
      gap: 10px;
      align-items: center;
      justify-content: center;
      flex-wrap: wrap;
    }
    button {
      padding: 10px 15px;
      font-size: 14px;
      cursor: pointer;
      background-color: #333;
      color: white;
      border: 1px solid #555;
      border-radius: 5px;
      transition: all 0.2s;
    }
    button:hover { background-color: #444; }
    button:active { transform: translateY(1px); }
    button.primary { background-color: #007bff; border-color: #007bff; }
    button.primary:hover { background-color: #0056b3; }
    button.danger { background-color: #dc3545; border-color: #dc3545; }
    button.danger:hover { background-color: #bd2130; }
    button.success { background-color: #28a745; border-color: #28a745; }
    button.success:hover { background-color: #218838; }
    
    input[type=range] { width: 150px; }
    .label { font-size: 14px; color: #aaa; }
    #frame-indicator { font-weight: bold; min-width: 80px; text-align: center; }
  </style>
</head>
<body>
  <h1>LED Matrix Animator</h1>
  
  <div class="grid-container" id="grid"></div>

  <div class="controls-container">
    <div class="row">
      <button onclick="prevFrame()">&lt; Prev</button>
      <span id="frame-indicator">Frame 1/1</span>
      <button onclick="nextFrame()">Next &gt;</button>
    </div>
    
    <div class="row">
      <button onclick="addFrame()" class="primary">New Frame</button>
      <button onclick="copyFrame()">Duplicate</button>
      <button onclick="deleteFrame()" class="danger">Delete</button>
      <button onclick="clearFrame()">Clear</button>
    </div>

    <div class="row" style="margin-top: 10px; border-top: 1px solid #333; padding-top: 15px;">
      <button id="playBtn" onclick="togglePlay()" class="success">Play Animation</button>
      <div style="display: flex; flex-direction: column; align-items: center; gap: 5px;">
        <span class="label">Speed: <span id="speedVal">200</span>ms</span>
        <input type="range" id="speed" min="50" max="1000" step="50" value="200" oninput="updateSpeed(this.value)">
      </div>
    </div>
    
    <div class="row">
       <button onclick="sendCurrentFrame()">Display Current Frame</button>
    </div>
  </div>

  <script>
    const rows = 8;
    const cols = 12;
    const grid = document.getElementById('grid');
    const frameIndicator = document.getElementById('frame-indicator');
    const playBtn = document.getElementById('playBtn');
    
    // State
    let frames = [];
    let currentFrameIndex = 0;
    let isPlaying = false;
    let playTimer = null;
    let animationSpeed = 200;

    // Initialize with one empty frame
    function createEmptyFrame() {
      return new Array(rows * cols).fill(0);
    }
    
    frames.push(createEmptyFrame());

    // Create Grid DOM
    function initGrid() {
        grid.innerHTML = '';
        for (let i = 0; i < rows * cols; i++) {
          let pixel = document.createElement('div');
          pixel.className = 'pixel';
          pixel.dataset.index = i;
          pixel.onmousedown = function(e) { e.preventDefault(); togglePixel(i); };
          pixel.onmouseover = function(e) { if(e.buttons === 1) setPixel(i, 1); };
          grid.appendChild(pixel);
        }
        renderGrid();
    }

    function togglePixel(index) {
        const frame = frames[currentFrameIndex];
        frame[index] = frame[index] ? 0 : 1;
        renderGrid();
        // If not playing, update immediately for feedback
        if (!isPlaying) sendCurrentFrame();
    }
    
    function setPixel(index, val) {
        const frame = frames[currentFrameIndex];
        if(frame[index] !== val) {
            frame[index] = val;
            renderGrid();
            if (!isPlaying && val === 1) sendCurrentFrame(); 
        }
    }

    function renderGrid() {
        const frame = frames[currentFrameIndex];
        const pixels = grid.children;
        for (let i = 0; i < pixels.length; i++) {
            if (frame[i]) pixels[i].classList.add('active');
            else pixels[i].classList.remove('active');
        }
        updateUI();
    }

    function updateUI() {
        frameIndicator.innerText = `Frame ${currentFrameIndex + 1}/${frames.length}`;
        playBtn.innerText = isPlaying ? "Stop Animation" : "Play Animation";
        playBtn.className = isPlaying ? "danger" : "success";
    }

    // Frame Management
    function addFrame() {
        frames.splice(currentFrameIndex + 1, 0, createEmptyFrame());
        currentFrameIndex++;
        renderGrid();
    }

    function copyFrame() {
        const newFrame = [...frames[currentFrameIndex]];
        frames.splice(currentFrameIndex + 1, 0, newFrame);
        currentFrameIndex++;
        renderGrid();
    }

    function deleteFrame() {
        if (frames.length > 1) {
            frames.splice(currentFrameIndex, 1);
            if (currentFrameIndex >= frames.length) currentFrameIndex = frames.length - 1;
            renderGrid();
        } else {
            clearFrame();
        }
    }

    function clearFrame() {
        frames[currentFrameIndex] = createEmptyFrame();
        renderGrid();
        sendCurrentFrame();
    }

    function prevFrame() {
        if (currentFrameIndex > 0) {
            currentFrameIndex--;
            renderGrid();
            sendCurrentFrame();
        }
    }

    function nextFrame() {
        if (currentFrameIndex < frames.length - 1) {
            currentFrameIndex++;
            renderGrid();
            sendCurrentFrame();
        }
    }

    // Playback
    function togglePlay() {
        if (isPlaying) stopAnimation();
        else startAnimation();
    }

    function startAnimation() {
        isPlaying = true;
        updateUI();
        playStep();
    }

    function stopAnimation() {
        isPlaying = false;
        clearTimeout(playTimer);
        updateUI();
    }

    function updateSpeed(val) {
        animationSpeed = parseInt(val);
        document.getElementById('speedVal').innerText = val;
    }

    async function playStep() {
        if (!isPlaying) return;

        // Send current frame
        await sendFrameData(frames[currentFrameIndex]);
        
        // Move to next frame
        currentFrameIndex = (currentFrameIndex + 1) % frames.length;
        renderGrid(); // Update UI to show progress

        // Schedule next step
        playTimer = setTimeout(playStep, animationSpeed);
    }

    // Network
    function sendCurrentFrame() {
        sendFrameData(frames[currentFrameIndex]);
    }

    function sendFrameData(frameData) {
        let pattern = "";
        for(let i=0; i<frameData.length; i++) pattern += frameData[i];
        
        // Return the fetch promise so we can await it if needed
        return fetch('/api/matrix', {
            method: 'POST',
            headers: {'Content-Type': 'text/plain'},
            body: pattern
        }).catch(err => console.error("Error sending frame:", err));
    }

    // Init
    initGrid();
  </script>
</body>
</html>
)rawliteral";