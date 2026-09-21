class Danmaku {
    constructor(options) {
        this.options = options;
        this.container = this.options.container;
        this.danTunnel = {
            right: {},
            top: {},
            bottom: {},
        };
        this.showing = true;
        this.context = document.createElement('canvas').getContext('2d');
    }

    /**
     * Push a danmaku into DPlayer
     *
     * @param {Object Array} dan - {text, color, type}
     * text - danmaku content without html encoding
     * color - danmaku color, default: `#fff`
     * type - danmaku type, `right` `top` `bottom`, default: `right`
     */
    draw(dan) {
        if (this.showing) {
            const itemHeight = Math.max(Math.min((this.options.heightPercent || 7) / 100 * this.container.offsetHeight,
                                                 this.options.maxHeightPx || Number.MAX_VALUE),
                                        Math.max(this.options.minHeightPx || 2, 2));
            const danWidth = this.container.offsetWidth;
            const danPaddingTop = (this.options.paddingTopPercent || 0) / 100 * this.container.offsetHeight;
            const danPaddingBottom = (this.options.paddingBottomPercent || 0) / 100 * this.container.offsetHeight;
            const danHeight = Math.max(this.container.offsetHeight - danPaddingTop - danPaddingBottom, 0);
            const itemY = parseInt(danHeight / itemHeight);

            const danItemRight = (ele) => {
                const eleWidth = ele.offsetWidth || parseInt(ele.style.width);
                const eleRight = ele.getBoundingClientRect().right || this.container.getBoundingClientRect().right + eleWidth;
                return this.container.getBoundingClientRect().right - eleRight;
            };

            const danSpeed = (width) => (danWidth + width) / (this.options.duration || 5);

            const getTunnel = (ele, type, width) => {
                const tmp = danWidth / danSpeed(width);

                for (let i = 0; this.options.unlimited || i < itemY; i++) {
                    const item = this.danTunnel[type][i + ''];
                    if (item && item.length) {
                        if (type !== 'right') {
                            continue;
                        }
                        for (let j = 0; j < item.length; j++) {
                            const danRight = danItemRight(item[j]) - 10;
                            if (danRight <= danWidth - tmp * danSpeed(parseInt(item[j].style.width)) || danRight <= 0) {
                                break;
                            }
                            if (j === item.length - 1) {
                                this.danTunnel[type][i + ''].push(ele);
                                ele.addEventListener('animationend', () => {
                                    this.danTunnel[type][i + ''].splice(0, 1);
                                });
                                return i % itemY;
                            }
                        }
                    } else {
                        this.danTunnel[type][i + ''] = [ele];
                        ele.addEventListener('animationend', () => {
                            this.danTunnel[type][i + ''].splice(0, 1);
                        });
                        return i % itemY;
                    }
                }
                return -1;
            };

            if (Object.prototype.toString.call(dan) !== '[object Array]') {
                dan = [dan];
            }

            const docFragment = document.createDocumentFragment();

            for (let i = 0; i < dan.length; i++) {
                if (!dan[i].color && dan[i].color !== 0) {
                    dan[i].color = 16777215;
                }
                const item = document.createElement('div');
                item.classList.add('dplayer-danmaku-item');
                item.classList.add(`dplayer-danmaku-${dan[i].type}`);
                if ((dan[i].color >> 16) * 3 + (dan[i].color >> 8) % 256 * 6 + dan[i].color % 256 < 255) {
                    item.classList.add('dplayer-danmaku-dark');
                }
                if (dan[i].border) {
                    const spanItem = document.createElement('span');
                    spanItem.style.border = dan[i].border;
                    spanItem.innerText = dan[i].text;
                    item.appendChild(spanItem);
                } else {
                    item.innerText = dan[i].text;
                }
                item.style.color = '#' + ('00000' + dan[i].color.toString(16)).slice(-6);
                item.style.fontSize = Math.floor(itemHeight * 0.8) + 'px';
                item.addEventListener('animationend', () => {
                    this.container.removeChild(item);
                });

                const itemWidth = this._measure(dan[i].text);
                let tunnel;

                // adjust
                switch (dan[i].type) {
                    case 'right':
                        tunnel = getTunnel(item, dan[i].type, itemWidth);
                        if (tunnel >= 0) {
                            item.style.width = itemWidth + 1 + 'px';
                            item.style.top = (danPaddingTop + itemHeight * tunnel) + 'px';
                            item.style.transform = `translateX(-${danWidth}px)`;
                        }
                        break;
                    case 'top':
                        tunnel = getTunnel(item, dan[i].type);
                        if (tunnel >= 0) {
                            item.style.top = (danPaddingTop + itemHeight * tunnel) + 'px';
                        }
                        break;
                    case 'bottom':
                        tunnel = getTunnel(item, dan[i].type);
                        if (tunnel >= 0) {
                            item.style.bottom = (danPaddingBottom + itemHeight * tunnel) + 'px';
                        }
                        break;
                    default:
                        console.error(`Can't handled danmaku type: ${dan[i].type}`);
                }

                if (tunnel >= 0) {
                    // move
                    item.classList.add('dplayer-danmaku-move');
                    item.style.animationDuration = this._danAnimation(dan[i].type);

                    // insert
                    docFragment.appendChild(item);
                }
            }

            this.container.appendChild(docFragment);

            return docFragment;
        }
    }

    _measure(text) {
        const itemHeight = Math.max(Math.min((this.options.heightPercent || 7) / 100 * this.container.offsetHeight,
                                             this.options.maxHeightPx || Number.MAX_VALUE),
                                    Math.max(this.options.minHeightPx || 2, 2));
        if (this.contextItemHeight !== itemHeight) {
            this.contextItemHeight = itemHeight;
            const item = document.createElement('div');
            item.classList.add('dplayer-danmaku-item');
            item.classList.add('dplayer-danmaku-item--demo');
            item.style.fontSize = Math.floor(itemHeight * 0.8) + 'px';
            this.container.appendChild(item);
            const measureStyle = getComputedStyle(item, null);
            this.context.font = measureStyle.getPropertyValue('font');
            this.container.removeChild(item);
        }
        const lines = text.split('\n');
        let maxWidth = 0;
        for (let i = 0; i < lines.length; i++) {
            maxWidth = Math.max(maxWidth, this.context.measureText(lines[i]).width);
        }
        return maxWidth;
    }

    clear() {
        this.danTunnel = {
            right: {},
            top: {},
            bottom: {},
        };
        const items = this.container.getElementsByClassName('dplayer-danmaku-item');
        while (items.length > 0) {
            this.container.removeChild(items[0]);
        }
    }

    resize() {
        const danWidth = this.container.offsetWidth;
        const items = this.container.getElementsByClassName('dplayer-danmaku-item');
        for (let i = 0; i < items.length; i++) {
            items[i].style.transform = `translateX(-${danWidth}px)`;
        }
    }

    hide() {
        this.showing = false;
        this.clear();
    }

    show() {
        this.clear();
        this.showing = true;
    }

    _danAnimation(position) {
        const animations = {
            top: `${(this.options.duration || 5) * 0.8}s`,
            right: `${(this.options.duration || 5)}s`,
            bottom: `${(this.options.duration || 5) * 0.8}s`,
        };
        return animations[position];
    }
}
