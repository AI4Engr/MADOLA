#include "html_formatter.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cmath>

namespace madola {

std::string HtmlFormatter::generateGraphHtml(const std::vector<GraphData>& graphs) {
    if (graphs.empty()) return "";

    std::stringstream html;
    // Remove the "Graphs" heading - just show the graphs directly

    for (size_t i = 0; i < graphs.size(); ++i) {
        const auto& graph = graphs[i];

        html << "<div class=\"graph-container\">\n";
        html << "<div class=\"graph-title\">" << escapeHtml(graph.title) << "</div>\n";
        html << "<div id=\"graph" << i << "\"></div>\n";
        html << "<script>\n";
        html << "(function() {\n";
        html << "    const data" << i << " = [";

        // Write data points
        for (size_t j = 0; j < graph.x_values.size(); ++j) {
            if (j > 0) html << ",";
            html << "{x:" << graph.x_values[j] << ",y:" << graph.y_values[j] << "}";
        }

        html << "];\n\n";
        html << "    const margin = {top: 20, right: 30, bottom: 40, left: 50};\n";
        html << "    const width = 800 - margin.left - margin.right;\n";
        html << "    const height = 400 - margin.top - margin.bottom;\n\n";

        html << "    const svg = d3.select(\"#graph" << i << "\")\n";
        html << "        .append(\"svg\")\n";
        html << "        .attr(\"width\", width + margin.left + margin.right)\n";
        html << "        .attr(\"height\", height + margin.top + margin.bottom);\n\n";

        html << "    const g = svg.append(\"g\")\n";
        html << "        .attr(\"transform\", `translate(${margin.left},${margin.top})`);\n\n";

        html << "    const xScale = d3.scaleLinear()\n";
        html << "        .domain(d3.extent(data" << i << ", d => d.x))\n";
        html << "        .range([0, width]);\n\n";

        html << "    const yScale = d3.scaleLinear()\n";
        html << "        .domain(d3.extent(data" << i << ", d => d.y))\n";
        html << "        .range([height, 0]);\n\n";

        html << "    const line = d3.line()\n";
        html << "        .x(d => xScale(d.x))\n";
        html << "        .y(d => yScale(d.y));\n\n";

        // Add axes
        html << "    g.append(\"g\")\n";
        html << "        .attr(\"class\", \"axis\")\n";
        html << "        .attr(\"transform\", `translate(0,${height})`)\n";
        html << "        .call(d3.axisBottom(xScale));\n\n";

        html << "    g.append(\"g\")\n";
        html << "        .attr(\"class\", \"axis\")\n";
        html << "        .call(d3.axisLeft(yScale));\n\n";

        // Add line
        html << "    g.append(\"path\")\n";
        html << "        .datum(data" << i << ")\n";
        html << "        .attr(\"class\", \"line\")\n";
        html << "        .attr(\"d\", line);\n\n";

        // Add dots
        html << "    g.selectAll(\".dot\")\n";
        html << "        .data(data" << i << ")\n";
        html << "        .enter().append(\"circle\")\n";
        html << "        .attr(\"class\", \"dot\")\n";
        html << "        .attr(\"cx\", d => xScale(d.x))\n";
        html << "        .attr(\"cy\", d => yScale(d.y))\n";
        html << "        .attr(\"r\", 3);\n";

        html << "})();\n";
        html << "</script>\n";
        html << "</div>\n";
    }

    return html.str();
}

std::string HtmlFormatter::generate3DGraphHtml(const Graph3DData& graph, size_t index) {
    std::stringstream html;

    html << "<div class=\"graph-container\">\n";
    html << "<div class=\"graph-title\">" << escapeHtml(graph.title) << "</div>\n";
    html << "<div id=\"graph3d" << index << "\" style=\"width: 100%; aspect-ratio: 4 / 3;\"></div>\n";
    html << "<script>\n";
    html << "(function() {\n";

    // Create Three.js scene for 3D brick with hole
    html << "    // Check if Three.js is available\n";
    html << "    if (typeof THREE === 'undefined') {\n";
    html << "        console.error('Three.js not loaded for 3D graph');\n";
    html << "        document.getElementById('graph3d" << index << "').innerHTML = '<p style=\"color: red;\">Three.js required for 3D graphs</p>';\n";
    html << "        return;\n";
    html << "    }\n\n";

    html << "    const container = document.getElementById('graph3d" << index << "');\n";
    html << "    if (!container) {\n";
    html << "        console.error('Container graph3d" << index << " not found');\n";
    html << "        return;\n";
    html << "    }\n\n";
    html << "    const scene = new THREE.Scene();\n";
    html << "    const renderer = new THREE.WebGLRenderer({ antialias: true });\n";
    html << "    const DPR = (window.devicePixelRatio || 1);\n";
    html << "    function getContainerSize() {\n";
    html << "        const rect = container.getBoundingClientRect();\n";
    html << "        const w = Math.max(1, Math.floor(rect.width));\n";
    html << "        const h = Math.max(1, Math.floor(rect.height));\n";
    html << "        return { width: w, height: h };\n";
    html << "    }\n";
    html << "    const size = getContainerSize();\n";
    html << "    const camera = new THREE.PerspectiveCamera(75, size.width / size.height, 0.1, 1000);\n";
    html << "    renderer.setPixelRatio(DPR);\n";
    html << "    renderer.setSize(size.width, size.height);\n";
    html << "    renderer.setClearColor(0xf0f0f0);\n";
    html << "    renderer.shadowMap.enabled = true;\n";
    html << "    renderer.shadowMap.type = THREE.PCFSoftShadowMap;\n";
    html << "    container.appendChild(renderer.domElement);\n";
    html << "    renderer.domElement.style.width = '100%';\n";
    html << "    renderer.domElement.style.height = '100%';\n\n";

    // Add lighting
    html << "    // Add lighting\n";
    html << "    const ambientLight = new THREE.AmbientLight(0x404040, 0.6);\n";
    html << "    scene.add(ambientLight);\n";
    html << "    const directionalLight = new THREE.DirectionalLight(0xffffff, 1);\n";
    html << "    directionalLight.position.set(10, 10, 5);\n";
    html << "    directionalLight.castShadow = true;\n";
    html << "    scene.add(directionalLight);\n\n";

    // Create brick with hole using CSG (simplified approach with Three.js)
    html << "    // Create brick with hole geometry\n";
    html << "    const brickWidth = " << graph.width << ";\n";
    html << "    const brickHeight = " << graph.height << ";\n";
    html << "    const brickDepth = " << graph.depth << ";\n";
    html << "    const holeWidth = " << graph.hole_width << ";\n";
    html << "    const holeHeight = " << graph.hole_height << ";\n";
    html << "    const holeDepth = " << graph.hole_depth << ";\n\n";

    // Create the outer brick
    html << "    const brickGeometry = new THREE.BoxGeometry(brickWidth, brickHeight, brickDepth);\n";
    html << "    const brickMaterial = new THREE.MeshLambertMaterial({ color: 0x8B4513, transparent: true, opacity: 0.8 });\n";
    html << "    const brick = new THREE.Mesh(brickGeometry, brickMaterial);\n";
    html << "    brick.castShadow = true;\n";
    html << "    brick.receiveShadow = true;\n\n";

    // Create the hole (for visualization, we'll create a frame-like structure)
    html << "    // Create hole visualization as inset geometry\n";
    html << "    const holeGeometry = new THREE.BoxGeometry(holeWidth, holeHeight, holeDepth + 0.1);\n";
    html << "    const holeMaterial = new THREE.MeshLambertMaterial({ color: 0x000000, transparent: true, opacity: 0.3 });\n";
    html << "    const hole = new THREE.Mesh(holeGeometry, holeMaterial);\n\n";

    // Combine them (simplified CSG approach)
    html << "    // Create brick frame structure to show hole\n";
    html << "    const group = new THREE.Group();\n";
    html << "    \n";
    html << "    // Bottom part\n";
    html << "    const bottomGeom = new THREE.BoxGeometry(brickWidth, (brickHeight - holeHeight) / 2, brickDepth);\n";
    html << "    const bottomMesh = new THREE.Mesh(bottomGeom, brickMaterial);\n";
    html << "    bottomMesh.position.y = -(brickHeight + holeHeight) / 4;\n";
    html << "    group.add(bottomMesh);\n";
    html << "    \n";
    html << "    // Top part\n";
    html << "    const topGeom = new THREE.BoxGeometry(brickWidth, (brickHeight - holeHeight) / 2, brickDepth);\n";
    html << "    const topMesh = new THREE.Mesh(topGeom, brickMaterial);\n";
    html << "    topMesh.position.y = (brickHeight + holeHeight) / 4;\n";
    html << "    group.add(topMesh);\n";
    html << "    \n";
    html << "    // Left part\n";
    html << "    const leftGeom = new THREE.BoxGeometry((brickWidth - holeWidth) / 2, holeHeight, brickDepth);\n";
    html << "    const leftMesh = new THREE.Mesh(leftGeom, brickMaterial);\n";
    html << "    leftMesh.position.x = -(brickWidth + holeWidth) / 4;\n";
    html << "    group.add(leftMesh);\n";
    html << "    \n";
    html << "    // Right part\n";
    html << "    const rightGeom = new THREE.BoxGeometry((brickWidth - holeWidth) / 2, holeHeight, brickDepth);\n";
    html << "    const rightMesh = new THREE.Mesh(rightGeom, brickMaterial);\n";
    html << "    rightMesh.position.x = (brickWidth + holeWidth) / 4;\n";
    html << "    group.add(rightMesh);\n";
    html << "    \n";
    html << "    // Front part\n";
    html << "    const frontGeom = new THREE.BoxGeometry(holeWidth, holeHeight, (brickDepth - holeDepth) / 2);\n";
    html << "    const frontMesh = new THREE.Mesh(frontGeom, brickMaterial);\n";
    html << "    frontMesh.position.z = (brickDepth + holeDepth) / 4;\n";
    html << "    group.add(frontMesh);\n";
    html << "    \n";
    html << "    // Back part\n";
    html << "    const backGeom = new THREE.BoxGeometry(holeWidth, holeHeight, (brickDepth - holeDepth) / 2);\n";
    html << "    const backMesh = new THREE.Mesh(backGeom, brickMaterial);\n";
    html << "    backMesh.position.z = -(brickDepth + holeDepth) / 4;\n";
    html << "    group.add(backMesh);\n\n";

    html << "    scene.add(group);\n\n";

    // Add wireframe overlay
    html << "    // Add wireframe overlay\n";
    html << "    const wireframeGeometry = new THREE.BoxGeometry(brickWidth, brickHeight, brickDepth);\n";
    html << "    const wireframeMaterial = new THREE.WireframeGeometry(wireframeGeometry);\n";
    html << "    const wireframe = new THREE.LineSegments(wireframeMaterial, new THREE.LineBasicMaterial({ color: 0x000000, linewidth: 2 }));\n";
    html << "    scene.add(wireframe);\n\n";

    // Position camera
    html << "    // Fit camera to object utility\n";
    html << "    function fitCameraToObject(object, padding = 1.1) {\n";
    html << "        const box = new THREE.Box3().setFromObject(object);\n";
    html << "        const center = box.getCenter(new THREE.Vector3());\n";
    html << "        const sizeVec = box.getSize(new THREE.Vector3());\n";
    html << "        const radius = 0.5 * Math.sqrt(sizeVec.x*sizeVec.x + sizeVec.y*sizeVec.y + sizeVec.z*sizeVec.z);\n";
    html << "        const fovy = THREE.MathUtils.degToRad(camera.fov);\n";
    html << "        const fovh = 2 * Math.atan(Math.tan(fovy/2) * camera.aspect);\n";
    html << "        const distV = (radius * padding) / Math.tan(fovy/2);\n";
    html << "        const distH = (radius * padding) / Math.tan(fovh/2);\n";
    html << "        const distance = Math.max(distV, distH);\n";
    html << "        return { center, distance };\n";
    html << "    }\n\n";
    html << "    // Initial camera setup\n";
    html << "    let cameraDistance = 12;\n";
    html << "    let targetCameraX = 0;\n";
    html << "    let targetCameraY = 0;\n";
    html << "    let cameraX = 0;\n";
    html << "    let cameraY = 0;\n";
    html << "    const fit = fitCameraToObject(group, 1.2);\n";
    html << "    cameraDistance = fit.distance;\n";
    html << "    const fitCenter = fit.center;\n";
    html << "    camera.position.set(fitCenter.x + cameraDistance, fitCenter.y + cameraDistance*0.75, fitCenter.z + cameraDistance);\n";
    html << "    camera.lookAt(fitCenter);\n\n";

    // Add mouse controls with zoom and pan
    html << "    // Enhanced mouse controls: rotation, zoom, and pan\n";
    html << "    let mouseDown = false;\n";
    html << "    let rightMouseDown = false;\n";
    html << "    let mouseX = 0;\n";
    html << "    let mouseY = 0;\n";
    html << "    let targetRotationX = 0;\n";
    html << "    let targetRotationY = 0;\n";
    html << "    let rotationX = 0;\n";
    html << "    let rotationY = 0;\n\n";

    html << "    function updateCameraPosition() {\n";
    html << "        const distance = cameraDistance;\n";
    html << "        const x = fitCenter.x + Math.cos(targetRotationY) * distance + cameraX;\n";
    html << "        const z = fitCenter.z + Math.sin(targetRotationY) * distance;\n";
    html << "        const y = fitCenter.y + Math.sin(targetRotationX) * distance + cameraY;\n";
    html << "        camera.position.set(x, y, z);\n";
    html << "        camera.lookAt(fitCenter.x + cameraX, fitCenter.y + cameraY, fitCenter.z);\n";
    html << "    }\n\n";

    html << "    container.addEventListener('mousedown', function(event) {\n";
    html << "        event.preventDefault();\n";
    html << "        if (event.button === 0) {\n";  // Left click
    html << "            mouseDown = true;\n";
    html << "        } else if (event.button === 2) {\n";  // Right click
    html << "            rightMouseDown = true;\n";
    html << "        }\n";
    html << "        mouseX = event.clientX;\n";
    html << "        mouseY = event.clientY;\n";
    html << "    });\n\n";

    html << "    container.addEventListener('mouseup', function(event) {\n";
    html << "        event.preventDefault();\n";
    html << "        if (event.button === 0) {\n";
    html << "            mouseDown = false;\n";
    html << "        } else if (event.button === 2) {\n";
    html << "            rightMouseDown = false;\n";
    html << "        }\n";
    html << "    });\n\n";

    html << "    container.addEventListener('contextmenu', function(event) {\n";
    html << "        event.preventDefault(); // Disable right-click context menu\n";
    html << "    });\n\n";

    html << "    container.addEventListener('mousemove', function(event) {\n";
    html << "        event.preventDefault();\n";
    html << "        if (mouseDown) {\n";
    html << "            // Rotation with left mouse button\n";
    html << "            const deltaX = event.clientX - mouseX;\n";
    html << "            const deltaY = event.clientY - mouseY;\n";
    html << "            targetRotationY += deltaX * 0.01;\n";
    html << "            targetRotationX += deltaY * 0.01;\n";
    html << "            targetRotationX = Math.max(-Math.PI/2, Math.min(Math.PI/2, targetRotationX));\n";
    html << "        } else if (rightMouseDown) {\n";
    html << "            // Pan with right mouse button\n";
    html << "            const deltaX = event.clientX - mouseX;\n";
    html << "            const deltaY = event.clientY - mouseY;\n";
    html << "            targetCameraX -= deltaX * 0.02;\n";
    html << "            targetCameraY += deltaY * 0.02;\n";
    html << "        }\n";
    html << "        mouseX = event.clientX;\n";
    html << "        mouseY = event.clientY;\n";
    html << "    });\n\n";

    // Zoom with mouse wheel
    html << "    // Zoom with mouse wheel\n";
    html << "    container.addEventListener('wheel', function(event) {\n";
    html << "        event.preventDefault();\n";
    html << "        const zoomSpeed = 0.1;\n";
    html << "        if (event.deltaY > 0) {\n";
    html << "            cameraDistance = Math.min(50, cameraDistance + zoomSpeed);\n";
    html << "        } else {\n";
    html << "            cameraDistance = Math.max(0.5, cameraDistance - zoomSpeed);\n";
    html << "        }\n";
    html << "    });\n\n";

    html << "    // Resize handling: keep renderer and camera in sync and auto-fit\n";
    html << "    function handleResize() {\n";
    html << "        const s = getContainerSize();\n";
    html << "        renderer.setSize(s.width, s.height);\n";
    html << "        camera.aspect = s.width / s.height;\n";
    html << "        camera.updateProjectionMatrix();\n";
    html << "        const newFit = fitCameraToObject(group, 1.2);\n";
    html << "        // Only update distance if container changed significantly\n";
    html << "        cameraDistance = newFit.distance;\n";
    html << "    }\n";
    html << "    if (typeof ResizeObserver !== 'undefined') {\n";
    html << "        const ro = new ResizeObserver(handleResize);\n";
    html << "        ro.observe(container);\n";
    html << "    } else {\n";
    html << "        window.addEventListener('resize', handleResize);\n";
    html << "    }\n\n";

    // Animation loop
    html << "    function animate() {\n";
    html << "        requestAnimationFrame(animate);\n";
    html << "        \n";
    html << "        // Smooth interpolation for all controls\n";
    html << "        rotationX += (targetRotationX - rotationX) * 0.1;\n";
    html << "        rotationY += (targetRotationY - rotationY) * 0.1;\n";
    html << "        cameraX += (targetCameraX - cameraX) * 0.1;\n";
    html << "        cameraY += (targetCameraY - cameraY) * 0.1;\n";
    html << "        \n";
    html << "        // Update camera position based on current values\n";
    html << "        updateCameraPosition();\n";
    html << "        \n";
    html << "        // Apply rotation to the object (not the camera)\n";
    html << "        group.rotation.x = 0;\n";
    html << "        group.rotation.y = 0;\n";
    html << "        wireframe.rotation.x = 0;\n";
    html << "        wireframe.rotation.y = 0;\n";
    html << "        \n";
    html << "        renderer.render(scene, camera);\n";
    html << "    }\n";
    html << "    \n";
    html << "    // Initialize camera position and ensure size fits\n";
    html << "    handleResize();\n";
    html << "    updateCameraPosition();\n";
    html << "    animate();\n\n";

    html << "})();\n";
    html << "</script>\n";
    html << "</div>\n";

    return html.str();
}

std::string HtmlFormatter::generateTableHtml(const std::vector<TableData>& tables) {
    if (tables.empty()) return "";

    std::stringstream html;

    for (size_t i = 0; i < tables.size(); ++i) {
        html << generateSingleTableHtml(tables[i], i);
    }

    return html.str();
}

std::string HtmlFormatter::generateSingleTableHtml(const TableData& table, size_t index) {
    std::stringstream html;

    html << "<div class=\"table-container\">\n";
    html << "<table class=\"data-table\" id=\"table" << index << "\">\n";

    // Table header
    html << "  <thead>\n";
    html << "    <tr>\n";
    for (const auto& header : table.headers) {
        html << "      <th>" << escapeHtml(header) << "</th>\n";
    }
    html << "    </tr>\n";
    html << "  </thead>\n";

    // Table body
    html << "  <tbody>\n";

    // Find max row count
    size_t maxRows = 0;
    for (const auto& col : table.columns) {
        if (std::holds_alternative<std::vector<double>>(col)) {
            maxRows = std::max(maxRows, std::get<std::vector<double>>(col).size());
        } else if (std::holds_alternative<std::vector<std::string>>(col)) {
            maxRows = std::max(maxRows, std::get<std::vector<std::string>>(col).size());
        }
    }

    // Generate rows
    for (size_t row = 0; row < maxRows; ++row) {
        html << "    <tr>\n";
        for (size_t col = 0; col < table.columns.size(); ++col) {
            html << "      <td>";

            if (std::holds_alternative<std::vector<double>>(table.columns[col])) {
                // Numeric column
                const auto& numCol = std::get<std::vector<double>>(table.columns[col]);
                if (row < numCol.size()) {
                    double val = numCol[row];
                    // Format number nicely
                    if (std::floor(val) == val && std::abs(val) < 1e15) {
                        html << static_cast<long long>(val);
                    } else {
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(3) << val;
                        std::string result = oss.str();
                        // Remove trailing zeros
                        result.erase(result.find_last_not_of('0') + 1, std::string::npos);
                        if (result.back() == '.') result.pop_back();
                        html << result;
                    }
                }
            } else if (std::holds_alternative<std::vector<std::string>>(table.columns[col])) {
                // String column
                const auto& strCol = std::get<std::vector<std::string>>(table.columns[col]);
                if (row < strCol.size()) {
                    html << escapeHtml(strCol[row]);
                }
            }

            html << "</td>\n";
        }
        html << "    </tr>\n";
    }

    html << "  </tbody>\n";
    html << "</table>\n";
    html << "</div>\n";

    return html.str();
}

std::string HtmlFormatter::generateMathContent(const Program& program, Evaluator& evaluator) {
    std::stringstream html;

    // Format each statement as mathematical content
    for (const auto& stmt : program.statements) {
        std::string mathExpr = formatStatementAsMath(*stmt, evaluator);
        if (!mathExpr.empty()) {
            html << "<div class=\"math-expression align-expression\">\n";
            html << "$$" << mathExpr << "$$\n";
            html << "</div>\n";
        }
    }

    return html.str();
}

std::string HtmlFormatter::formatStatementAsMath(const Statement& stmt, Evaluator& evaluator) {
    if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(&stmt)) {
        // Format assignment as math - use original expression for readability
        std::string varName = convertToMathJax(assignment->variable);
        // For most expressions, show the structure; for arrays, show the values
        std::string exprStr;
        if (dynamic_cast<const ArrayExpression*>(assignment->expression.get())) {
            // For arrays (matrices/vectors), show the actual values
            exprStr = formatExpressionWithValuesAsMath(*assignment->expression, evaluator);
        } else {
            // For other expressions, show the original structure
            exprStr = formatExpressionAsMath(*assignment->expression, evaluator);
        }
        std::string result = varName + " = " + exprStr;
        if (!assignment->inlineComment.empty()) {
            if (assignment->commentBefore) {
                // |- means comment before expression in output
                result = "\\text{" + assignment->inlineComment + "} \\quad " + result;
            } else {
                // -| means comment after expression in output
                result = result + " \\quad \\text{" + assignment->inlineComment + "}";
            }
        }
        return result;
    } else if (const auto* forStmt = dynamic_cast<const ForStatement*>(&stmt)) {
        // Format for loop with algorithmic structure like markdown
        std::stringstream ss;
        ss << "\\begin{array}{l}\n";
        ss << "\\textbf{for } " << convertToMathJax(forStmt->variable) << " \\textbf{ in } ";

        // Format the range properly
        if (const auto* rangeExpr = dynamic_cast<const RangeExpression*>(forStmt->range.get())) {
            if (rangeExpr->start && rangeExpr->end) {
                ss << formatExpressionAsMath(*rangeExpr->start, evaluator) << "..." << formatExpressionAsMath(*rangeExpr->end, evaluator);
            } else {
                ss << "INVALID_RANGE";
            }
        } else if (forStmt->range) {
            ss << formatExpressionAsMath(*forStmt->range, evaluator);
        } else {
            ss << "NULL_RANGE";
        }

        ss << " \\\\\n";
        ss << "\\left|\n";
        ss << "\\begin{array}{l}\n";

        for (size_t i = 0; i < forStmt->body.size(); ++i) {
            const auto* decoratedStmt = dynamic_cast<const DecoratedStatement*>(forStmt->body[i].get());

            // Check if this statement has @layoutNxM decorator (e.g., @layout1x2)
            if (decoratedStmt && decoratedStmt->hasLayoutDecorator()) {
                const Decorator* arrayDec = decoratedStmt->getLayoutDecorator();
                // Check if it's a 1xN layout decorator (single row, multiple columns)
                if (arrayDec && arrayDec->rows == 1 && arrayDec->cols >= 2) {
                    // Format the decorated statement and the next (cols-1) statements on the same line
                    if (decoratedStmt->statement && i + arrayDec->cols - 1 < forStmt->body.size()) {
                        std::vector<std::string> statements;
                        statements.push_back(formatStatementAsMathInFunction(*decoratedStmt->statement, evaluator, 0));

                        // Collect the next (cols-1) statements
                        for (int j = 1; j < arrayDec->cols; ++j) {
                            statements.push_back(formatStatementAsMathInFunction(*forStmt->body[i + j], evaluator, 0));
                        }

                        // Check if all statements formatted successfully
                        bool allValid = true;
                        for (const auto& stmt : statements) {
                            if (stmt.empty()) {
                                allValid = false;
                                break;
                            }
                        }

                        if (allValid) {
                            // Output all statements on the same line with extra spacing
                            ss << "\\quad ";
                            for (size_t j = 0; j < statements.size(); ++j) {
                                if (j > 0) ss << " \\qquad ";  // Use \qquad for more space between equations
                                ss << statements[j];
                            }
                            if (i + arrayDec->cols - 1 < forStmt->body.size() - 1) {
                                ss << " \\\\\n";
                            } else {
                                ss << "\n";
                            }
                            i += arrayDec->cols - 1; // Skip the statements we already processed
                        }
                    }
                }
            } else {
                std::string bodyMath = formatStatementAsMathInFunction(*forStmt->body[i], evaluator, 0);
                if (!bodyMath.empty()) {
                    ss << "\\quad " << bodyMath;
                    if (i < forStmt->body.size() - 1) {
                        ss << " \\\\\n";
                    } else {
                        ss << "\n";
                    }
                }
            }
        }

        ss << "\\end{array}\n";
        ss << "\\right.\n";
        ss << "\\end{array}";
        return ss.str();
    } else if (const auto* whileStmt = dynamic_cast<const WhileStatement*>(&stmt)) {
        // Format while loop with algorithmic structure like for loops
        std::stringstream ss;
        ss << "\\begin{array}{l}\n";
        ss << "\\textbf{while } " << formatExpressionAsMath(*whileStmt->condition, evaluator) << " \\textbf{ do} \\\\\n";
        ss << "\\left|\n";
        ss << "\\begin{array}{l}\n";

        for (size_t i = 0; i < whileStmt->body.size(); ++i) {
            std::string bodyMath = formatStatementAsMathInFunction(*whileStmt->body[i], evaluator, 0);
            if (!bodyMath.empty()) {
                ss << "\\quad " << bodyMath;
                if (i < whileStmt->body.size() - 1) {
                    ss << " \\\\\n";
                } else {
                    ss << "\n";
                }
            }
        }

        ss << "\\end{array}\n";
        ss << "\\right.\n";
        ss << "\\end{array}";
        return ss.str();
    } else if (const auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(&stmt)) {
        // Format function declaration with full structure like for loops
        std::stringstream ss;
        ss << "\\begin{array}{l}\n";
        ss << "\\textbf{function } " << funcDecl->name << "(";

        // Add parameters
        for (size_t i = 0; i < funcDecl->parameters.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << convertToMathJax(funcDecl->parameters[i]);
        }
        ss << ") \\\\\n";
        ss << "\\left|\n";
        ss << "\\begin{array}{l}\n";

        // Format the function body statements
        for (size_t i = 0; i < funcDecl->body.size(); ++i) {
            const auto& bodyStmt = funcDecl->body[i];
            const auto* decoratedStmt = dynamic_cast<const DecoratedStatement*>(bodyStmt.get());

            // Check if this statement has @layoutNxM decorator
            if (decoratedStmt && decoratedStmt->hasLayoutDecorator()) {
                const Decorator* arrayDec = decoratedStmt->getLayoutDecorator();
                // Check if it's a 1xN layout decorator (single row, multiple columns)
                if (arrayDec && arrayDec->rows == 1 && arrayDec->cols >= 2) {
                    // Format the decorated statement and the next (cols-1) statements on the same line
                    if (decoratedStmt->statement && i + arrayDec->cols - 1 < funcDecl->body.size()) {
                        std::vector<std::string> statements;
                        statements.push_back(formatStatementAsMathInFunction(*decoratedStmt->statement, evaluator, 0));

                        // Collect the next (cols-1) statements
                        for (int j = 1; j < arrayDec->cols; ++j) {
                            statements.push_back(formatStatementAsMathInFunction(*funcDecl->body[i + j], evaluator, 0));
                        }

                        // Check if all statements formatted successfully
                        bool allValid = true;
                        for (const auto& stmt : statements) {
                            if (stmt.empty()) {
                                allValid = false;
                                break;
                            }
                        }

                        if (allValid) {
                            // Output all statements on the same line with extra spacing
                            ss << "\\quad ";
                            for (size_t j = 0; j < statements.size(); ++j) {
                                if (j > 0) ss << " \\qquad ";
                                ss << statements[j];
                            }
                            if (i + arrayDec->cols - 1 < funcDecl->body.size() - 1) {
                                ss << " \\\\\n";
                            } else {
                                ss << "\n";
                            }
                            i += arrayDec->cols - 1; // Skip the statements we already processed
                        }
                    }
                }
            } else {
                std::string bodyMath = formatStatementAsMathInFunction(*bodyStmt, evaluator, 0);
                if (!bodyMath.empty()) {
                    ss << "\\quad " << bodyMath;
                    if (i < funcDecl->body.size() - 1) {
                        ss << " \\\\\n";
                    } else {
                        ss << "\n";
                    }
                }
            }
        }

        ss << "\\end{array}\n";
        ss << "\\right.\n";
        ss << "\\end{array}";
        return ss.str();
    } else if (const auto* piecewiseFuncDecl = dynamic_cast<const PiecewiseFunctionDeclaration*>(&stmt)) {
        // Format piecewise function declaration in LaTeX
        std::stringstream ss;
        ss << piecewiseFuncDecl->name << "(";
        for (size_t i = 0; i < piecewiseFuncDecl->parameters.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << piecewiseFuncDecl->parameters[i];
        }
        ss << ") = ";
        ss << formatPiecewiseExpressionAsMath(*piecewiseFuncDecl->piecewise, evaluator);
        return ss.str();
    } else if (const auto* ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
        // Format standalone if statement with algorithmic structure
        std::stringstream ss;
        ss << "\\begin{array}{l}\n";
        ss << "\\textbf{if } " << formatExpressionAsMath(*ifStmt->condition, evaluator) << " \\textbf{ then} \\\\\n";
        ss << "\\left|\n";
        ss << "\\begin{array}{l}\n";

        for (size_t i = 0; i < ifStmt->then_body.size(); ++i) {
            std::string bodyMath = formatStatementAsMathInFunction(*ifStmt->then_body[i], evaluator, 0);
            if (!bodyMath.empty()) {
                ss << "\\quad " << bodyMath;
                if (i < ifStmt->then_body.size() - 1) {
                    ss << " \\\\\n";  // Add LaTeX line break between statements
                } else {
                    ss << "\n";
                }
            }
        }

        ss << "\\end{array}\n";
        ss << "\\right.\n";

        // Handle else clause if present
        if (!ifStmt->else_body.empty()) {
            ss << "\\\\\n";
            ss << "\\textbf{else} \\\\\n";
            ss << "\\left|\n";
            ss << "\\begin{array}{l}\n";

            for (size_t i = 0; i < ifStmt->else_body.size(); ++i) {
                std::string bodyMath = formatStatementAsMathInFunction(*ifStmt->else_body[i], evaluator, 0);
                if (!bodyMath.empty()) {
                    ss << "\\quad " << bodyMath;
                    if (i < ifStmt->else_body.size() - 1) {
                        ss << " \\\\\n";  // Add LaTeX line break between statements
                    } else {
                        ss << "\n";
                    }
                }
            }

            ss << "\\end{array}\n";
            ss << "\\right.\n";
        }

        ss << "\\end{array}";
        return ss.str();
    } else if (const auto* decoratedStmt = dynamic_cast<const DecoratedStatement*>(&stmt)) {
        // Handle decorated statements with various decorators
        if (decoratedStmt->hasDecorator("resolve")) {
            if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(decoratedStmt->statement.get())) {
                std::stringstream ss;
                ss << convertToMathJax(assignment->variable) << " = " << formatExpressionAsMath(*assignment->expression, evaluator);

                // Add = and show variable values substituted
                ss << " = ";
                ss << formatExpressionWithValuesAsMath(*assignment->expression, evaluator);

                // Add = and show final result
                ss << " = ";
                Value result = evaluator.evaluateExpression(*assignment->expression);
                ss << formatValueAsMath(result);

                return ss.str();
            }
        } else if (decoratedStmt->hasDecorator("eval")) {
            if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(decoratedStmt->statement.get())) {
                std::stringstream ss;
                ss << convertToMathJax(assignment->variable) << " = " << formatExpressionAsMath(*assignment->expression, evaluator);

                // For @eval statements, we need to evaluate the expression with the PRE-assignment value
                // Create a fresh evaluator and execute all statements before this one
                ss << " = ";
                if (currentProgram && currentStatement) {
                    Evaluator freshEvaluator;
                    for (const auto& prevStmt : currentProgram->statements) {
                        if (prevStmt.get() == currentStatement) {
                            break; // Stop before the current statement
                        }
                        // Execute previous statements to build up environment
                        std::vector<std::string> dummyOutputs;
                        try {
                            freshEvaluator.executeStatement(*prevStmt, dummyOutputs);
                        } catch (...) {
                            // Ignore errors
                        }
                    }
                    // Now evaluate with the fresh evaluator
                    Value result = freshEvaluator.evaluateExpression(*assignment->expression);

                    // If result is a string (symbolic expression), format it as LaTeX
#ifdef WITH_SYMENGINE
                    if (std::holds_alternative<std::string>(result)) {
                        // For symbolic expressions, parse and format as LaTeX
                        std::string symbolicStr = std::get<std::string>(result);

                        // Clean up formatting: remove .0 from numbers
                        size_t pos = 0;
                        while ((pos = symbolicStr.find(".0", pos)) != std::string::npos) {
                            // Check if followed by * or ^ or end of string
                            if (pos + 2 >= symbolicStr.length() ||
                                symbolicStr[pos + 2] == '*' ||
                                symbolicStr[pos + 2] == ' ') {
                                symbolicStr.erase(pos, 2);
                            } else {
                                pos++;
                            }
                        }

                        // Remove ^1.0 or ^1 (x^1 = x)
                        pos = 0;
                        while ((pos = symbolicStr.find("**1.0", pos)) != std::string::npos) {
                            symbolicStr.erase(pos, 5);
                        }
                        pos = 0;
                        while ((pos = symbolicStr.find("**1", pos)) != std::string::npos) {
                            // Make sure it's not **10, **11, etc.
                            if (pos + 3 >= symbolicStr.length() || !isdigit(symbolicStr[pos + 3])) {
                                symbolicStr.erase(pos, 3);
                            } else {
                                pos++;
                            }
                        }

                        // Replace ** with ^
                        pos = 0;
                        while ((pos = symbolicStr.find("**", pos)) != std::string::npos) {
                            symbolicStr.replace(pos, 2, "^");
                            pos += 1;
                        }

                        // Replace * with \cdot
                        pos = 0;
                        while ((pos = symbolicStr.find("*", pos)) != std::string::npos) {
                            symbolicStr.replace(pos, 1, " \\cdot ");
                            pos += 7;
                        }

                        ss << symbolicStr;
                    } else {
                        ss << formatValueAsMath(result);
                    }
#else
                    ss << formatValueAsMath(result);
#endif
                } else {
                    // Fallback to old behavior if context not available
                    Value result = evaluator.evaluateExpression(*assignment->expression);

                    // If result is a string (symbolic expression), format it as LaTeX
#ifdef WITH_SYMENGINE
                    if (std::holds_alternative<std::string>(result)) {
                        // For symbolic expressions, parse and format as LaTeX
                        std::string symbolicStr = std::get<std::string>(result);

                        // Clean up formatting: remove .0 from numbers
                        size_t pos = 0;
                        while ((pos = symbolicStr.find(".0", pos)) != std::string::npos) {
                            // Check if followed by * or ^ or end of string
                            if (pos + 2 >= symbolicStr.length() ||
                                symbolicStr[pos + 2] == '*' ||
                                symbolicStr[pos + 2] == ' ') {
                                symbolicStr.erase(pos, 2);
                            } else {
                                pos++;
                            }
                        }

                        // Remove ^1.0 or ^1 (x^1 = x)
                        pos = 0;
                        while ((pos = symbolicStr.find("**1.0", pos)) != std::string::npos) {
                            symbolicStr.erase(pos, 5);
                        }
                        pos = 0;
                        while ((pos = symbolicStr.find("**1", pos)) != std::string::npos) {
                            // Make sure it's not **10, **11, etc.
                            if (pos + 3 >= symbolicStr.length() || !isdigit(symbolicStr[pos + 3])) {
                                symbolicStr.erase(pos, 3);
                            } else {
                                pos++;
                            }
                        }

                        // Replace ** with ^
                        pos = 0;
                        while ((pos = symbolicStr.find("**", pos)) != std::string::npos) {
                            symbolicStr.replace(pos, 2, "^");
                            pos += 1;
                        }

                        // Replace * with \cdot
                        pos = 0;
                        while ((pos = symbolicStr.find("*", pos)) != std::string::npos) {
                            symbolicStr.replace(pos, 1, " \\cdot ");
                            pos += 7;
                        }

                        ss << symbolicStr;
                    } else {
                        ss << formatValueAsMath(result);
                    }
#else
                    ss << formatValueAsMath(result);
#endif
                }

                return ss.str();
            } else if (const auto* exprStmt = dynamic_cast<const ExpressionStatement*>(decoratedStmt->statement.get())) {
                // Handle @eval on expression statements like "a;"
                std::stringstream ss;
                ss << formatExpressionAsMath(*exprStmt->expression, evaluator);
                ss << " = ";

                // Use fresh evaluator to get current state
                if (currentProgram && currentStatement) {
                    Evaluator freshEvaluator;
                    for (const auto& prevStmt : currentProgram->statements) {
                        if (prevStmt.get() == currentStatement) {
                            break; // Stop before the current statement
                        }
                        // Execute previous statements to build up environment
                        std::vector<std::string> dummyOutputs;
                        try {
                            freshEvaluator.executeStatement(*prevStmt, dummyOutputs);
                        } catch (...) {
                            // Ignore errors
                        }
                    }
                    Value result = freshEvaluator.evaluateExpression(*exprStmt->expression);

                    // If result is a string (symbolic expression), evaluate it symbolically
#ifdef WITH_SYMENGINE
                    if (std::holds_alternative<std::string>(result)) {
                        try {
                            result = freshEvaluator.evaluateSymbolicExpression(std::get<std::string>(result));
                        } catch (...) {
                            // If symbolic evaluation fails, keep the string
                        }
                    }
#endif
                    ss << formatValueAsMath(result);
                } else {
                    Value result = evaluator.evaluateExpression(*exprStmt->expression);

                    // If result is a string (symbolic expression), evaluate it symbolically
#ifdef WITH_SYMENGINE
                    if (std::holds_alternative<std::string>(result)) {
                        try {
                            result = evaluator.evaluateSymbolicExpression(std::get<std::string>(result));
                        } catch (...) {
                            // If symbolic evaluation fails, keep the string
                        }
                    }
#endif
                    ss << formatValueAsMath(result);
                }

                return ss.str();
            }
        } else if (decoratedStmt->hasDecorator("resolveAlign")) {
            if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(decoratedStmt->statement.get())) {
                std::stringstream ss;
                ss << "\\begin{align}\n";
                ss << convertToMathJax(assignment->variable) << " &= " << formatExpressionAsMath(*assignment->expression, evaluator);

                // Add = and show variable values substituted (aligned)
                ss << " \\\\\n";
                ss << "&= ";
                ss << formatExpressionWithValuesAsMath(*assignment->expression, evaluator);

                // Add = and show final result (aligned)
                ss << " \\\\\n";
                ss << "&= ";
                Value result = evaluator.evaluateExpression(*assignment->expression);
                ss << formatValueAsMath(result);

                ss << "\n\\end{align}";
                return ss.str();
            }
        } else {
            // For other decorators, format the underlying statement
            if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(decoratedStmt->statement.get())) {
                std::string varName = convertToMathJax(assignment->variable);
                std::string exprStr = formatExpressionAsMath(*assignment->expression, evaluator);
                return varName + " = " + exprStr;
            }
        }
    } else if (const auto* func = dynamic_cast<const FunctionCall*>(&stmt)) {
        // Skip graph() and graph_3d() and table() calls as they're handled separately
        if (func->function_name == "graph" || func->function_name == "graph_3d" || func->function_name == "table") {
            return "";
        }
    } else if (dynamic_cast<const CommentStatement*>(&stmt)) {
        // Skip comment formatting for math expressions - handle in generateOrderedContent instead
        return "";
    } else if (dynamic_cast<const PrintStatement*>(&stmt)) {
        // Print statements are not formatted as math blocks - shown in execution results
        return "";
    } else if (const auto* exprStmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        // Format standalone expression statements (e.g., comparisons: x < y;)
        // Just show the expression itself, without evaluating it
        return formatExpressionAsMath(*exprStmt->expression, evaluator);
    }

    return "";
}

} // namespace madola
